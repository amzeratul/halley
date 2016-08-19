#include "test_stage.h"

using namespace Halley;

TestStage::TestStage()
{
}

void TestStage::init()
{

}

void TestStage::onFixedUpdate(Time time)
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
			network = std::make_unique<NetworkService>(4113);
			network->setAcceptingConnections(true);
			std::cout << "Listening..." << std::endl;
		}
		else if (key->isButtonPressed(Keys::C)) {
			// Client
			network = std::make_unique<NetworkService>(0);
			setConnection(network->connect("127.0.0.1", 4113));			
			std::cout << "Connecting as client." << std::endl;
		}
	} else {
		network->update();

		if (connection) {
			if (connection->getStatus() != ConnectionStatus::OPEN) {
				std::cout << "Closing connection." << std::endl;
				connection.reset();
				return;
			}

			if (key->isButtonPressed(Keys::Space)) {
				connection->send(OutboundNetworkPacket(gsl::ensure_z("hello world!")));
			}

			if (connection->getTimeSinceLastSend() > 0.01f) {
				connection->send(OutboundNetworkPacket(gsl::ensure_z("no-op")));
			}

			InboundNetworkPacket received;
			while (connection->receive(received)) {
				gsl::byte buffer[2048];
				size_t bytes = received.copyTo(buffer);
				buffer[bytes] = gsl::byte(0);
				std::cout << "Received message: " << reinterpret_cast<const char*>(buffer) << std::endl;
			}

			if (connection->getTimeSinceLastReceive() > 2.0f) {
				connection->close();
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
	auto base = unstable ? std::make_shared<InstabilitySimulator>(conn, 0.1f, 0.05f, 0.1f, 0.05f) : conn;
	connection = std::make_shared<ReliableConnection>(base);
}
