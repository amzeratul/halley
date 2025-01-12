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

		InspectorEntityInfo() = default;
		InspectorEntityInfo(const ConfigNode& node);
		InspectorEntityInfo(ConstEntityRef entity);

		ConfigNode toConfigNode() const;

		bool operator==(const InspectorEntityInfo& other) const;
		bool operator!=(const InspectorEntityInfo& other) const;
	};

	struct InspectorWorldData {
		Vector<InspectorEntityInfo> entities;

		InspectorWorldData() = default;
		InspectorWorldData(const ConfigNode& node);
		InspectorWorldData(const World& world);

		ConfigNode toConfigNode() const;

		bool operator==(const InspectorWorldData& other) const;
		bool operator!=(const InspectorWorldData& other) const;
	};

	struct InspectorEntityData {
		Vector<InspectorEntityInfo> entities;

		InspectorEntityData() = default;
		InspectorEntityData(const ConfigNode& node);
		InspectorEntityData(EntityRef entity);

		ConfigNode toConfigNode() const;
	};

	class InspectorClient {
	public:
		InspectorClient(DevConClient& devcon);
		~InspectorClient();

		void update(gsl::span<World*> worlds);

	private:
		DevConClient& devcon;

		ConfigNode getInspectorData(const ConfigNode& params, gsl::span<World*> worlds);
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
		InspectorWorldData worldData;

		void onData(ConfigNode data);
	};
}
