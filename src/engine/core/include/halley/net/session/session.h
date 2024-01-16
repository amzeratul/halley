#pragma once
#include "halley/maths/rect.h"
#include "halley/data_structures/hash_map.h"
#include "halley/data_structures/config_node.h"

namespace Halley {
	class EntityNetworkSession;
	class NetworkSession;

	class Session {
	public:
		virtual ~Session() = default;

		virtual bool isMultiplayer() const { return false; }
		virtual bool hasLocalSave() const { return true; }
		virtual bool isReadyToStart() const { return true; }
		virtual bool hasHostAuthority() const { return true; }
		virtual size_t getNumberOfLocalPlayers() const { return 1; }
		virtual size_t getNumberOfPlayers() const { return 1; }
		virtual Vector<Rect4f> getRemoteViewPorts() const { return {}; }

		virtual bool isWaitingForInitialViewPort() const { return false; }
		virtual void reportInitialViewPort(Rect4f viewPort) {}

		virtual EntityNetworkSession* getEntityNetworkSession() { return nullptr; }
		virtual NetworkSession* getNetworkSession() { return nullptr; }

		virtual const String& getPlayerName() const { return String::emptyString(); }

		[[nodiscard]] virtual bool update() { return true; }

		ConfigNode& getOptions() { return options; }
		const ConfigNode& getOptions() const { return options; }

	private:
		ConfigNode options;
	};
}
