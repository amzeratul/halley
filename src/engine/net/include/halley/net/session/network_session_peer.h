#pragma once
#include "../connection/iconnection.h"
#include <memory>

namespace Halley {
	class NetworkSessionPeer {
	public:
		std::shared_ptr<IConnection> connection;
	};
}
