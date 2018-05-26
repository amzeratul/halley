#include "devcon/devcon_client.h"
#include "halley/net/connection/network_service.h"
#include "halley/net/connection/message_queue_tcp.h"
#include "halley/support/logger.h"
#include "halley/core/api/halley_api.h"
#include "halley/net/connection/message_queue.h"
#include "devcon/devcon_messages.h"

using namespace Halley;

DevConClient::DevConClient(const HalleyAPI& api, std::unique_ptr<NetworkService> service, const String& address, int port)
	: api(api)
	, service(std::move(service))
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
	for (auto& id: msg.getIds()) {
		auto splitPos = id.find(':');
		auto type = fromString<AssetType>(id.left(splitPos));
		String name = id.mid(splitPos + 1);

		Logger::logInfo("Reloading " + id);
		api.core->getResources().ofType(type).reload(name);
	}
}

void DevConClient::connect()
{
	connection = service->connect(address, port);
	queue = std::make_shared<MessageQueueTCP>(connection);
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
