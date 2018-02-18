#include "devcon/devcon_client.h"
#include "connection/network_service.h"
#include "halley/support/logger.h"
using namespace Halley;

DevConClient::DevConClient(std::unique_ptr<NetworkService> service, const String& address, int port)
	: service(std::move(service))
	, address(address)
	, port(port)
{
	connect();

	Logger::addSink(*this);
}

DevConClient::~DevConClient()
{
}

void DevConClient::update()
{
	service->update();
}

void DevConClient::connect()
{
	connection = std::make_shared<ReliableConnection>(service->connect(address, port));
	queue = std::make_shared<MessageQueue>(connection);
	queue->setChannel(0, ChannelSettings(true, true));
}

void DevConClient::log(LoggerLevel level, const String& msg)
{
	//queue->enqueue();
}
