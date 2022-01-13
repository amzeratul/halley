#include "connection/network_service.h"

using namespace Halley;

std::shared_ptr<IConnection> NetworkService::Acceptor::accept()
{
	if (!choiceMade) {
		choiceMade = true;
		return doAccept();
	} else {
		return {};
	}
}

void NetworkService::Acceptor::reject()
{
	if (!choiceMade) {
		choiceMade = true;
		doReject();
	}
}

void NetworkService::Acceptor::ensureChoiceMade()
{
	reject();
}
