#pragma once

#include "halley/data_structures/listener_set.h"
#include "halley/entity/service.h"
#include "halley/net/session/session.h"
#include "halley/net/session/session_multiplayer.h"
namespace Halley {

	class SessionService : public Service {
	public:
		struct ChangeData {
			std::shared_ptr<Session> oldSession;
			std::shared_ptr<Session> newSession;
		};
		using ChangeCallback = std::function<void(ChangeData)>;

		SessionService() = default;
		SessionService(std::shared_ptr<Session> session);

		Session& getSession() const;
		void setSession(std::shared_ptr<Session> newSession);

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
		ListenerSet<ChangeData> callbacks;
	};
}

using SessionService = Halley::SessionService;
