#include "halley/ui/behaviours/ui_fullscreen_behaviour.h"

#include "halley/ui/ui_widget.h"

using namespace Halley;

void UIFullscreenBehaviour::onAddedToRoot(UIRoot& root)
{
	getWidget()->setMinSize(root.getRect().getSize());
}

void UIFullscreenBehaviour::update(Time time)
{
	getWidget()->setMinSize(getWidget()->getRoot()->getRect().getSize());
}
