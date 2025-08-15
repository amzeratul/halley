#pragma once
#include "devcon_messages.h"
#include "halley/concurrency/future.h"
#include "halley/data_structures/config_node.h"
#include "halley/net/connection/iconnection.h"

namespace Halley {
	class UIDebugConsoleController;
	class MessageQueue;

	class DevConConnection {
    public:
		using RPCHandle = std::function<Future<ConfigNode>(ConfigNode)>;

		virtual ~DevConConnection();

		void setConnection(std::shared_ptr<IConnection> connection);
		String getAddress() const;
		bool isAlive() const;

		virtual void update(Time t);

		Future<ConfigNode> sendRPC(String method, ConfigNode params);
		void setRPCHandle(const String& method, RPCHandle handle);
		void setDebugConsoleController(std::shared_ptr<UIDebugConsoleController> consoleController);

	protected:
		virtual void onReceiveMessage(DevCon::LogMsg& msg) {}
		virtual void onReceiveMessage(DevCon::NotifyInterestMsg& msg) {}
		virtual void onReceiveMessage(DevCon::SetClientDataMsg& msg) {}
		virtual void onReceiveMessage(const DevCon::ReloadAssetsMsg& msg) {}
		virtual void onReceiveMessage(DevCon::RegisterInterestMsg& msg) {}
		virtual void onReceiveMessage(DevCon::UpdateInterestMsg& msg) {}
		virtual void onReceiveMessage(const DevCon::UnregisterInterestMsg& msg) {}
		virtual void onReceiveMessage(DevCon::UpdateStringsMsg& msg) {}

		virtual void onReceiveMessage(DevCon::RPCMsg& msg);
		virtual void onReceiveMessage(DevCon::RPCReplyMsg& msg);

		std::shared_ptr<MessageQueue> queue;

    private:
        std::shared_ptr<IConnection> connection;

        HashMap<String, RPCHandle> rpcHandles;
		uint64_t rpcId = 0;
		HashMap<uint64_t, Promise<ConfigNode>> pendingRPC;
    };
}
