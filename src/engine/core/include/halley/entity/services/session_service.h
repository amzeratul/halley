#pragma once

#include "halley/entity/service.h"
#include "halley/net/session/session.h"
#include "halley/net/session/session_multiplayer.h"
namespace Halley {

	class SessionService : public Service {
	public:
		SessionService() = default;
		SessionService(std::shared_ptr<Session> session);

		Session& getSession() const;
		const std::shared_ptr<Session>& getSessionPtr() const;
		SessionMultiplayer& getMultiplayerSession() const;

		bool isMultiplayer() const;
		bool canSave() const;
		bool hasHostAuthority() const;
		bool hasEntityAuthority(const EntityRef& entity) const;
		bool hasEntityAuthority(EntityId entityId) const;

		String getSessionClientName() const;
		uint8_t getMyClientId() const;

	private:
		std::shared_ptr<Session> session;
	};
}

using SessionService = Halley::SessionService;
