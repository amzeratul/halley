#include "xbl_manager.h"
#include "halley/support/logger.h"
#include "halley/text/halleystring.h"
#include "halley/concurrency/concurrent.h"

#include <map>

#include <vccorlib.h>
#include <winrt/base.h>
#include <winrt/Windows.System.UserProfile.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Gaming.XboxLive.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include "xsapi/services.h"
#include <ppltasks.h>

using namespace Halley;

template <typename T>
T from_cx(Platform::Object^ from)
{
    T to{ nullptr };

    winrt::check_hresult(reinterpret_cast<::IUnknown*>(from)
        ->QueryInterface(winrt::guid_of<T>(),
            reinterpret_cast<void**>(winrt::put_abi(to))));

    return to;
}

template <typename T>
T^ to_cx(winrt::Windows::Foundation::IUnknown const& from)
{
    return safe_cast<T^>(reinterpret_cast<Platform::Object^>(winrt::get_abi(from)));
}

XBLManager::XBLManager()
{
	
}

XBLManager::~XBLManager()
{
	deInit();
}

void XBLManager::init()
{
	signIn();
}

void XBLManager::deInit()
{
}

Bytes XBLManager::getSaveData(SaveDataType type, const String& path)
{
	if (!gameSaveContainer) {
		throw Exception("Unable to load save data, gameSaveContainer is not available!");
	}

	winrt::Windows::Storage::Streams::IBuffer buffer;

	//winrt::Windows::Foundation::Collections::IMap<winrt::hstring, winrt::Windows::Storage::Streams::IBuffer> updates;
	//updates.Insert(winrt::hstring(path.getUTF16()), buffer);

	auto key = winrt::hstring(path.getUTF16());
	std::vector<winrt::hstring> updates;
	updates.push_back(key);

	auto gameBlob = gameSaveContainer->GetAsync(winrt::single_threaded_vector<winrt::hstring>(std::move(updates)).GetView()).get();
	//gameSaveContainer->ReadAsync(updates.GetView()).get(); // Fuck it
	if (gameBlob.Status() == winrt::Windows::Gaming::XboxLive::Storage::GameSaveErrorStatus::Ok) {
		if (gameBlob.Value().HasKey(key)) {
			auto buffer = gameBlob.Value().Lookup(key);

			auto size = buffer.Length();
			Bytes result(size);
			auto dataReader = winrt::Windows::Storage::Streams::DataReader::FromBuffer(buffer);
			dataReader.ReadBytes(winrt::array_view<uint8_t>(result));

			return result;
		}
	}

	return {};
}

void XBLManager::setSaveData(SaveDataType type, const String& path, const Bytes& data)
{
	if (!gameSaveContainer) {
		throw Exception("Unable to set save data, gameSaveContainer is not available!");
	}
	
	auto dataWriter = winrt::Windows::Storage::Streams::DataWriter();
	dataWriter.WriteBytes(winrt::array_view<const uint8_t>(data));

	std::map<winrt::hstring, winrt::Windows::Storage::Streams::IBuffer> updates;
	updates[winrt::hstring(path.getUTF16())] = dataWriter.DetachBuffer();
	auto view = winrt::single_threaded_map(std::move(updates)).GetView();

	gameSaveContainer->SubmitUpdatesAsync(view, {}, L"");
}

void XBLManager::signIn()
{
	using namespace xbox::services::system;
	xboxUser = std::make_shared<xbox_live_user>(nullptr);
	auto dispatcher = to_cx<Platform::Object>(winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread().Dispatcher());

	xboxUser->signin_silently(dispatcher).then([=] (xbox::services::xbox_live_result<sign_in_result> result) -> winrt::Windows::Foundation::IAsyncAction
	{
		if (result.err()) {
			Logger::logError(String("Error signing in to Xbox Live: ") + result.err_message());
		} else {
			bool loggedIn = false;
			switch (result.payload().status()) {
			case success:
				loggedIn = true;
				break;

			case user_interaction_required:
				xboxUser->signin(dispatcher).then([&](xbox::services::xbox_live_result<sign_in_result> loudResult)
                {
                    if (!loudResult.err()) {
                        auto resPayload = loudResult.payload();
                        switch (resPayload.status()) {
                        case success:
                            loggedIn = true;
                            break;
                        case user_cancel:
                            break;
                        }
                    }
                }, concurrency::task_continuation_context::use_current());
				break;
			}

			if (loggedIn) {
				xboxLiveContext = std::make_shared<xbox::services::xbox_live_context>(xboxUser);

				xbox_live_user::add_sign_out_completed_handler([this](const sign_out_completed_event_args&)
				{
					xboxUser.reset();
					xboxLiveContext.reset();
				});
			}
		}

		co_await getConnectedStorage();
		Logger::logInfo("Storage thread done.");
	});
}

winrt::Windows::Foundation::IAsyncAction XBLManager::getConnectedStorage()
{
	using namespace winrt::Windows::Gaming::XboxLive::Storage;
	
	auto windowsUser = co_await winrt::Windows::System::User::FindAllAsync();

	GameSaveProviderGetResult result = co_await GameSaveProvider::GetForUserAsync(*windowsUser.First(), xboxLiveContext->application_config()->scid());

	if (result.Status() == GameSaveErrorStatus::Ok) {
		gameSaveProvider = result.Value();
		Logger::logInfo("Got game save provider");

		gameSaveContainer = gameSaveProvider.get().CreateContainer(L"save");
	}
}
