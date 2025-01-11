#pragma once
#include "entity.h"
#include "halley/data_structures/config_node.h"

namespace Halley {
	class DevConServerConnection;
	class DevConClient;

	struct InspectorWorldInfo {
		String name;
		UUID uuid;

		InspectorWorldInfo() = default;
		InspectorWorldInfo(const ConfigNode& node);
		InspectorWorldInfo(const World& world);

		ConfigNode toConfigNode() const;

		bool operator==(const InspectorWorldInfo& other) const;
		bool operator!=(const InspectorWorldInfo& other) const;
	};

	struct InspectorEntityInfo {
		EntityId id;
		EntityId parentId;
		WorldPartitionId partition;
		String name;
	};

	struct InspectorWorldData {
		Vector<InspectorEntityInfo> entities;
	};

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
		void setParams(ConfigNode params);

		using WorldInfoCallback = std::function<void(const Vector<InspectorWorldInfo>&)>;
		using WorldDataCallback = std::function<void(const InspectorWorldData&)>;

		void setWorldInfoCallback(WorldInfoCallback callback);
		void setWorldDataCallback(WorldDataCallback callback);

	private:
		std::shared_ptr<DevConServerConnection> connection;
		bool listening = false;
		std::optional<uint32_t> interestHandle;
		ConfigNode params;

		WorldInfoCallback worldInfoCallback;
		WorldDataCallback worldDataCallback;

		Vector<InspectorWorldInfo> worldInfos;

		void onData(ConfigNode data);
	};
}
