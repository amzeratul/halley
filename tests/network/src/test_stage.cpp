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
			network = std::make_unique<NetworkService>(4114);
			connection = network->connect("127.0.0.1", 4113);
			std::cout << "Connecting as client." << std::endl;
		}
	}
	else {
		network->update();

		auto conn = network->tryAcceptConnection();
		if (conn) {
			connection = conn;
			std::cout << "Client connected." << std::endl;
		}

		if (connection) {
			if (key->isButtonPressed(Keys::Space)) {
				connection->send(NetworkPacket("hello world!", 13));
				std::cout << "Sent packet from game." << std::endl;
			}

			NetworkPacket received;
			while (connection->receive(received)) {
				std::cout << "Got packet on game." << std::endl;
			}
		}

		network->update();
	}
}
