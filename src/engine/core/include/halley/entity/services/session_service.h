#pragma once

#include "halley/net/session/session.h"
#include "halley/net/session/session_multiplayer.h"

namespace Halley {

	class SessionService : public Service {
	public:
		SessionService() = default;
		SessionService(std::shared_ptr<Session> session);

		std::shared_ptr<Session> getSession() const;
		SessionMultiplayer& getMultiplayerSession();

		bool isMultiplayer() const;
		bool canSave() const;
		bool hasHostAuthority() const;

	private:
		std::shared_ptr<Session> session;
	};
}

using SessionService = Halley::SessionService;
