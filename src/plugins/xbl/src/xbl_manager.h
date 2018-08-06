#pragma once

#include <memory>
#include <winrt/base.h>
#include <winrt/Windows.Gaming.XboxLive.Storage.h>
#include "halley/data_structures/maybe.h"
#include "halley/utils/utils.h"
#include <halley/core/api/save_data.h>

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

		std::shared_ptr<ISaveData> getSaveContainer(const String& name);

	private:
		std::shared_ptr<xbox::services::system::xbox_live_user> xboxUser;
		std::shared_ptr<xbox::services::xbox_live_context> xboxLiveContext;
		Maybe<winrt::Windows::Gaming::XboxLive::Storage::GameSaveProvider> gameSaveProvider;
		std::map<String, std::shared_ptr<ISaveData>> saveStorage;

		void signIn();
		winrt::Windows::Foundation::IAsyncAction getConnectedStorage();
	};

	class XBLSaveData : public ISaveData {
	public:
		explicit XBLSaveData(winrt::Windows::Gaming::XboxLive::Storage::GameSaveContainer container);

		bool isReady() const override;
		Bytes getData(const String& path) override;
		std::vector<String> enumerate(const String& root) override;
		void setData(const String& path, const Bytes& data, bool commit) override;
		void commit() override;

	private:
		winrt::Windows::Gaming::XboxLive::Storage::GameSaveContainer gameSaveContainer;
	};
}
