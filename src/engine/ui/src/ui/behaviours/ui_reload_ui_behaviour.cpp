#include "halley/ui/behaviours/ui_reload_ui_behaviour.h"

#include "ui_factory.h"
#include "ui_widget.h"

using namespace Halley;

UIReloadUIBehaviour::UIReloadUIBehaviour(UIFactory& factory, ConfigObserver observer)
	: factory(factory)
	, observer(observer)
{}

void UIReloadUIBehaviour::update(Time time)
{
	if (observer.needsUpdate()) {
		observer.update();

		getWidget()->clear();
		getWidget()->add(factory.makeUIFromNode(observer.getRoot()), 1);
		getWidget()->onMakeUI();
	}
}
