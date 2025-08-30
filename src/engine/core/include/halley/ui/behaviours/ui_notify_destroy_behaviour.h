#pragma once

#include "../ui_behaviour.h"

namespace Halley {
	class UIAnchor;

	class UINotifyDestroyBehaviour final : public UIBehaviour {
	public:
        using Callback = std::function<void()>;
        UINotifyDestroyBehaviour(Callback callback);
		void onParentDestroyed() override;

    private:
        Callback callback;
	};
}
