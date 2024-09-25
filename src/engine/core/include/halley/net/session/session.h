#pragma once
#include "halley/maths/rect.h"
#include "halley/data_structures/hash_map.h"
#include "halley/data_structures/config_node.h"
#include "halley/maths/uuid.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class EntityNetworkSession;
	class NetworkSession;

	class Session {
	public:
		Session()
		{
			options.ensureType(ConfigNodeType::Map);
		}
		virtual ~Session() = default;

		virtual bool isInteractive() const { return true; }
		virtual bool isMultiplayer() const { return false; }
		virtual bool hasLocalSave() const { return true; }
		virtual bool hasHostAuthority() const { return true; }
		virtual size_t getNumberOfLocalPlayers() const { return 1; }
		virtual size_t getNumberOfPlayers() const { return 1; }
		virtual uint8_t getMyClientId() const { return 0; }
		virtual Vector<Rect4f> getRemoteViewPorts() const { return {}; }

		virtual EntityNetworkSession* getEntityNetworkSession() { return nullptr; }
		virtual NetworkSession* getNetworkSession() { return nullptr; }

		virtual const String& getPlayerName() const { return String::emptyString(); }

		[[nodiscard]] virtual bool update(Time t) { return true; }

		ConfigNode& getOptions() { return options; }
		const ConfigNode& getOptions() const { return options; }

		virtual UUID getSessionSeed() const { return {}; }

	private:
		ConfigNode options;
	};
}
