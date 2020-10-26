#include "halley/ui/behaviours/ui_reload_ui_behaviour.h"

#include "ui_factory.h"
#include "ui_widget.h"
#include "halley/support/logger.h"

using namespace Halley;

UIReloadUIBehaviour::UIReloadUIBehaviour(UIFactory& factory, ConfigObserver observer)
	: factory(factory)
	, observer(observer)
{}

void UIReloadUIBehaviour::update(Time time)
{
	if (observer.needsUpdate()) {
		observer.update();

		try {
			auto ui = factory.makeUIFromNode(observer.getRoot());

			// The above might throw, don't clear until after we know it hasn't
			getWidget()->clear();
			getWidget()->add(std::move(ui), 1);
			getWidget()->onMakeUI();
		} catch (const std::exception& e) {
			Logger::logException(e);
		}
	}
}
