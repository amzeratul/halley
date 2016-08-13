#include "test_stage.h"

using namespace Halley;

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

	if (!networkInit) {
		if (key->isButtonPressed(Keys::S)) {
			// Server
			network.startListening(4113);
			networkInit = true;
			std::cout << "Listening..." << std::endl;
		}
		else if (key->isButtonPressed(Keys::C)) {
			// Client
			connection = network.connect("127.0.0.1", 4113);
			networkInit = true;
			std::cout << "Connecting as client." << std::endl;
		}
	}
	else {
		auto conn = network.tryAcceptConnection();
		if (conn) {
			connection = conn;
			std::cout << "Client connected." << std::endl;
		}

		if (connection) {
			if (key->isButtonPressed(Keys::Space)) {
				connection->send(NetworkPacket());
				std::cout << "Sent packet." << std::endl;
			}

			NetworkPacket received;
			while (connection->receive(received)) {
				std::cout << "Got packet." << std::endl;
			}
		}
	}
}
