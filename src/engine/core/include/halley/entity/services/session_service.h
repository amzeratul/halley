#pragma once

#include "halley/data_structures/listener_set.h"
#include "halley/entity/service.h"
#include "halley/net/session/session.h"
#include "halley/net/session/session_multiplayer.h"
namespace Halley {

	class SessionService : public Service {
	public:
		using ChangeCallback = std::function<void(std::shared_ptr<Session>)>;

		SessionService() = default;
		SessionService(std::shared_ptr<Session> session);

		Session& getSession() const;
		void setSession(std::shared_ptr<Session> session);

		const std::shared_ptr<Session>& getSessionPtr() const;
		SessionMultiplayer& getMultiplayerSession() const;

		bool isMultiplayer() const;
		bool canSave() const;
		bool hasHostAuthority() const;
		bool hasEntityAuthority(const EntityRef& entity) const;
		bool hasEntityAuthority(EntityId entityId) const;

		String getSessionClientName() const;
		uint8_t getMyClientId() const;

		[[nodiscard]] ListenerSetToken addSessionChangeCallback(ChangeCallback callback);

	private:
		std::shared_ptr<Session> session;
		ListenerSet<std::shared_ptr<Session>> callbacks;
	};
}

using SessionService = Halley::SessionService;
