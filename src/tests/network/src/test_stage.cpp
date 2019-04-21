#include "test_stage.h"
#include "halley/text/string_converter.h"
#include "halley/net/connection/iconnection.h"
#include "halley/net/connection/network_message.h"
#include "halley/net/connection/message_queue_tcp.h"

using namespace Halley;


class NoOpMsg : public NetworkMessage
{
public:
	NoOpMsg(gsl::span<const gsl::byte> src) {}

	void serialize(Serializer&) const override {}
};

class TextMsg : public NetworkMessage
{
public:
	TextMsg()
	{}

	TextMsg(String str)
		: str(str)
	{}

	TextMsg(gsl::span<const gsl::byte> src) {
		Deserializer d(src);
		d >> str;
	}

	String getString() const { return str; }

	void serialize(Serializer& s) const override
	{
		s << str;
	}

private:
	String str;
};


TestStage::TestStage()
{
}

void TestStage::init()
{

}

void TestStage::onVariableUpdate(Time)
{
	auto key = getInputAPI().getKeyboard();
	if (key->isButtonDown(Keys::Esc)) {
		getCoreAPI().quit();
	}

	updateNetwork();
}

void TestStage::onRender(RenderContext& context) const
{
	context.bind([&] (Painter& painter)
	{
		painter.clear(Colour(0));
	});
}

void TestStage::updateNetwork()
{
	auto key = getInputAPI().getKeyboard();

	if (!network) {
		if (key->isButtonPressed(Keys::S)) {
			// Server
			isClient = false;
			network = getNetworkAPI().createService(NetworkProtocol::TCP, 4113);
			network->setAcceptingConnections(true);
			std::cout << "Listening..." << std::endl;
		}
		else if (key->isButtonPressed(Keys::C)) {
			// Client
			isClient = true;
			network = getNetworkAPI().createService(NetworkProtocol::TCP, 0);
			setConnection(network->connect("127.0.0.1", 4113));			
			std::cout << "Connecting as client." << std::endl;
		}
	} else {
		network->update();

		if (connection) {
			if (connection->getStatus() == ConnectionStatus::Closed) {
				std::cout << "Closing connection." << std::endl;
				connection.reset();
				if (isClient) {
					network.reset();
				}
				return;
			} else {
				if (connection->getTimeSinceLastReceive() > 2.0f) {
					connection->close();
				}
			}
			
			if (connection->getStatus() == ConnectionStatus::Connected) {
				if (key->isButtonPressed(Keys::Space)) {
					msgs->enqueue(std::make_unique<TextMsg>("ding"), 1);
				}

				if (connection->getTimeSinceLastSend() > 0.01f) {
					//msgs->enqueue(std::make_unique<NoOpMsg>(), 0);
					size_t n = Random::getGlobal().getInt(1, 5);
					for (size_t i = 0; i < n; i++) {
						msgs->enqueue(std::make_unique<TextMsg>(toString(count++)), 2);
					}
				}

				for (auto& msg: msgs->receiveAll()) {
					if (dynamic_cast<NoOpMsg*>(msg.get())) {
						//std::cout << ".";
					} else {
						auto text = dynamic_cast<TextMsg*>(msg.get());
						if (text) {
							std::cout << text->getString() << "\n";
						}
					}
				}
				msgs->sendAll();
			}
		} else {
			auto conn = network->tryAcceptConnection();
			if (conn) {
				setConnection(conn);
				std::cout << "Client connected." << std::endl;
			}
		}

		network->update();
	}
}

void TestStage::setConnection(std::shared_ptr<Halley::IConnection> conn)
{
	const bool unstable = true;
	auto base = unstable ? std::make_shared<InstabilitySimulator>(conn, 0.1f, 0.03f, 0.1f, 0.05f) : conn;
	connection = std::make_shared<ReliableConnection>(base);
	
	msgs = std::make_unique<MessageQueueTCP>(connection);
	msgs->setChannel(0, ChannelSettings(false, false, false));
	msgs->setChannel(1, ChannelSettings(true, false, false));
	msgs->setChannel(2, ChannelSettings(false, true, false));
	msgs->setChannel(3, ChannelSettings(true, true, false));
	msgs->addFactory<NoOpMsg>();
	msgs->addFactory<TextMsg>();
}
