#pragma once

#include <memory>
#include <winrt/base.h>
#include <winrt/Windows.Gaming.XboxLive.Storage.h>
#include "halley/data_structures/maybe.h"

namespace xbox {
	namespace services {
		class xbox_live_context;
		namespace system {
			class xbox_live_user;
		}
	}
}

namespace Halley {
	class XBLManager {
	public:
		XBLManager();
		~XBLManager();

		void init();
		void deInit();

	private:
		std::shared_ptr<xbox::services::system::xbox_live_user> xboxUser;
		std::shared_ptr<xbox::services::xbox_live_context> xboxLiveContext;
		Maybe<winrt::Windows::Gaming::XboxLive::Storage::GameSaveProvider> gameSaveProvider;

		void signIn();
		winrt::Windows::Foundation::IAsyncAction getConnectedStorage();
	};
}
