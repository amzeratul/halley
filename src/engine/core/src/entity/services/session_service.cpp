#include "halley/entity/services/session_service.h"

using namespace Halley;

SessionService::SessionService(std::shared_ptr<Session> session)
	: session(std::move(session))
{
}

Session& SessionService::getSession() const
{
	assert(session);
	return *session;
}

void SessionService::setSession(std::shared_ptr<Session> session)
{
	this->session = std::move(session);
	callbacks.notify(this->session);
}

const std::shared_ptr<Session>& SessionService::getSessionPtr() const
{
	return session;
}

bool SessionService::isMultiplayer() const
{
	return session ? session->isMultiplayer() : false;
}

bool SessionService::canSave() const
{
	return session ? session->hasLocalSave() && session->isInteractive() : false;
}

bool SessionService::hasHostAuthority() const
{
	return session ? session->hasHostAuthority() : true;
}

bool SessionService::hasEntityAuthority(const EntityRef& entity) const
{
	if (session && entity.isValid()) {
		uint8_t myPeerId = getMyClientId();
		if (const auto authority = entity.getAuthorityPeerId()) {
			return authority == myPeerId;
		} else {
			return entity.getOwnerPeerId().value_or(myPeerId) == myPeerId;
		}
	}
	return hasHostAuthority();
}

bool SessionService::hasEntityAuthority(EntityId entityId) const
{
	if (session) {
		return hasEntityAuthority(getWorld().tryGetEntity(entityId));
	}
	return hasHostAuthority();
}

String SessionService::getSessionClientName() const
{
	if (isMultiplayer()) {
		auto& mp = getMultiplayerSession();
		if (mp.hasHostAuthority()) {
			return "host";
		} else {
			return "client" + toString(int(mp.getMyClientId()));
		}
	} else {
		return "local";
	}
}

uint8_t SessionService::getMyClientId() const
{
	return isMultiplayer() ? getMultiplayerSession().getMyClientId() : 0;
}

ListenerSetToken SessionService::addSessionChangeCallback(ChangeCallback callback)
{
	return callbacks.add(std::move(callback));
}

SessionMultiplayer& SessionService::getMultiplayerSession() const
{
	return dynamic_cast<SessionMultiplayer&>(*session);
}
