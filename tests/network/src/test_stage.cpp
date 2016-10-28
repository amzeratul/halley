#include "test_stage.h"
#include "halley/text/string_converter.h"

using namespace Halley;


class NoOpMsg : public NetworkMessage
{
public:
	size_t getSerializedSize() const override { return 0; }	
	void serializeTo(gsl::span<gsl::byte> dst) const override {}	
	void deserializeFrom(gsl::span<const gsl::byte> src) override {}
};

class TextMsg : public NetworkMessage
{
public:
	TextMsg()
	{}

	TextMsg(String str)
		: str(str)
	{}

	String getString() const { return str; }

	size_t getSerializedSize() const override
	{
		return str.size();
	}
	
	void serializeTo(gsl::span<gsl::byte> dst) const override
	{
		memcpy(dst.data(), str.c_str(), str.size());
	}
	
	void deserializeFrom(gsl::span<const gsl::byte> src) override
	{
		str = String(reinterpret_cast<const char*>(src.data()), src.size());
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

void TestStage::onVariableUpdate(Time time)
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
			network = std::make_unique<NetworkService>(4113);
			network->setAcceptingConnections(true);
			std::cout << "Listening..." << std::endl;
		}
		else if (key->isButtonPressed(Keys::C)) {
			// Client
			isClient = true;
			network = std::make_unique<NetworkService>(0);
			setConnection(network->connect("127.0.0.1", 4113));			
			std::cout << "Connecting as client." << std::endl;
		}
	} else {
		network->update();

		if (connection) {
			if (connection->getStatus() == ConnectionStatus::CLOSED) {
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
			
			if (connection->getStatus() == ConnectionStatus::OPEN) {
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
	
	msgs = std::make_unique<MessageQueue>(connection);
	msgs->setChannel(0, ChannelSettings(false, false, false));
	msgs->setChannel(1, ChannelSettings(true, false, false));
	msgs->setChannel(2, ChannelSettings(false, true, false));
	msgs->setChannel(3, ChannelSettings(true, true, false));
	msgs->addFactory<NoOpMsg>();
	msgs->addFactory<TextMsg>();
}
