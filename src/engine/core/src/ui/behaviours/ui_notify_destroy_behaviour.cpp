#include "halley/ui/behaviours/ui_notify_destroy_behaviour.h"

using namespace Halley;

UINotifyDestroyBehaviour::UINotifyDestroyBehaviour(Callback callback)
	: callback(std::move(callback))
{}

void UINotifyDestroyBehaviour::onParentDestroyed()
{
	callback();
}
