#pragma once
#include "entity.h"
#include "halley/data_structures/config_node.h"

namespace Halley {
	class DevConServerConnection;
	class DevConClient;

	class InspectorClient {
	public:
		InspectorClient(DevConClient& devcon);
		~InspectorClient();

		void update(gsl::span<World*> worlds);

	private:
		DevConClient& devcon;

		ConfigNode getInspectorData(const ConfigNode& params, gsl::span<World*> worlds);
		ConfigNode getWorldData(World& world);
		ConfigNode getEntityData(EntityRef entity);
	};

	class InspectorServer {
	public:
		InspectorServer(std::shared_ptr<DevConServerConnection> connection);
		~InspectorServer();

		void setListening(bool listening);

	private:
		std::shared_ptr<DevConServerConnection> connection;
		bool listening = false;
		std::optional<uint32_t> interestHandle;

		void onData(ConfigNode data);
	};
}
