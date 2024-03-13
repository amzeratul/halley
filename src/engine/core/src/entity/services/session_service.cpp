#include "halley/entity/services/session_service.h"

using namespace Halley;

SessionService::SessionService(std::shared_ptr<Session> session)
	: session(std::move(session))
{
}

std::shared_ptr<Session> SessionService::getSession() const
{
	return session;
}

bool SessionService::isMultiplayer() const
{
	return session ? session->isMultiplayer() : false;
}

bool SessionService::canSave() const
{
	return session ? session->hasLocalSave() : false;
}

bool SessionService::hasHostAuthority() const
{
	return session ? session->hasHostAuthority() : true;
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

SessionMultiplayer& SessionService::getMultiplayerSession() const
{
	return dynamic_cast<SessionMultiplayer&>(*session);
}
