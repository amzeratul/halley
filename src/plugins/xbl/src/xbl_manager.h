#pragma once

#include <memory>

namespace xbox {
	namespace services {
		namespace system {
			class xbox_live_user;
		}
		class xbox_live_context;
	}
}

namespace Halley {
	class XBLManager {
	public:
		XBLManager();
		~XBLManager();

	private:
		std::shared_ptr<xbox::services::system::xbox_live_user> xboxUser;
		std::shared_ptr<xbox::services::xbox_live_context> xboxLiveContext;
	};
}
