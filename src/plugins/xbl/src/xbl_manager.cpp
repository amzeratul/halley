#include "xbl_manager.h"
#include "xsapi/services.h"
#include "halley/support/logger.h"
#include "halley/text/halleystring.h"
#include <Windows.UI.Core.h>

#include "xsapi/services.h"

using namespace xbox::services::system;
using namespace Halley;

XBLManager::XBLManager()
{
	
}

XBLManager::~XBLManager()
{
	deInit();
}

void XBLManager::init()
{
	xboxUser = std::make_shared<xbox_live_user>(nullptr);
	auto dispatcher = ::Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher;

	xboxUser->signin_silently(dispatcher).then([=] (xbox::services::xbox_live_result<sign_in_result> result)
	{
		if (result.err()) {
			Logger::logError(String("Error signing in to Xbox Live: ") + result.err_message());
		} else {
			bool loggedIn = false;
			switch (result.payload().status()) {
			case sign_in_status::success:
				loggedIn = true;
				break;

			case sign_in_status::user_interaction_required:
				xboxUser->signin(dispatcher).then([&](xbox::services::xbox_live_result<xbox::services::system::sign_in_result> loudResult)
                {
                    if (!loudResult.err()) {
                        auto resPayload = loudResult.payload();
                        switch (resPayload.status()) {
                        case xbox::services::system::sign_in_status::success:
                            loggedIn = true;
                            break;
                        case xbox::services::system::sign_in_status::user_cancel:
                            break;
                        }
                    }
                }, concurrency::task_continuation_context::use_current());
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
	});
}

void XBLManager::deInit()
{
}
