#include "devcon/devcon_client.h"
#include "halley/net/connection/network_service.h"
#include "halley/net/connection/message_queue_tcp.h"
#include "halley/support/logger.h"
#include "halley/core/api/halley_api.h"
#include "halley/net/connection/message_queue.h"
#include "devcon/devcon_messages.h"

using namespace Halley;

DevConClient::DevConClient(const HalleyAPI& api, Resources& resources, std::unique_ptr<NetworkService> service, String address, int port)
	: api(api)
	, resources(resources)
	, service(std::move(service))
	, address(std::move(address))
	, port(port)
{
	connect();

	Logger::addSink(*this);
}

DevConClient::~DevConClient()
{
	Logger::removeSink(*this);
	queue.reset();
	service.reset();
}

void DevConClient::update()
{
	service->update();

	for (auto& m: queue->receiveAll()) {
		auto& msg = dynamic_cast<DevCon::DevConMessage&>(*m);
		switch (msg.getMessageType()) {
		case DevCon::MessageType::Log:
			// TODO?
			break;

		case DevCon::MessageType::ReloadAssets:
			onReceiveReloadAssets(dynamic_cast<DevCon::ReloadAssetsMsg&>(msg));
			break;

		default:
			break;
		}
	}
}

void DevConClient::onReceiveReloadAssets(const DevCon::ReloadAssetsMsg& msg)
{
	resources.reloadAssets(msg.getIds());
}

void DevConClient::connect()
{
	queue = std::make_shared<MessageQueueTCP>(service->connect(address + ":" + toString(port)));
	DevCon::setupMessageQueue(*queue);
}

void DevConClient::log(LoggerLevel level, const String& msg)
{
	if (level != LoggerLevel::Dev) {
		if (queue->isConnected()) {
			queue->enqueue(std::make_unique<DevCon::LogMsg>(level, msg), 0);
		}
	}
}
