#include "halley/ui/behaviours/ui_reload_ui_behaviour.h"

#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/support/logger.h"
#include "halley/ui/ui_definition.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

UIReloadUIBehaviour::UIReloadUIBehaviour(UIFactory& factory, ResourceObserver observer, IUIReloadObserver* reloadObserver)
	: factory(factory)
	, observer(observer)
	, reloadObserver(reloadObserver)
{}

void UIReloadUIBehaviour::init()
{
	setupUIStyleObservers();
}

void UIReloadUIBehaviour::update(Time time)
{
	if (observer.needsUpdate() || requireStyleUpdate()) {
		observer.update();

		try {
			auto ui = factory.makeUI(*dynamic_cast<const UIDefinition*>(observer.getResourceBeingObserved()));

			// The above might throw, don't clear until after we know it hasn't
			auto& widget = *getWidget();
			widget.clearChildren(); // Don't clear behaviours!!
			widget.add(std::move(ui), 1);
			widget.onMakeUI();
			widget.layout();

			if (reloadObserver) {
				reloadObserver->onOtherUIReloaded(widget);
			}

			setupUIStyleObservers(); // Different styles may be used, so need to reset
		} catch (const std::exception& e) {
			Logger::logException(e);
		}
	}
}

bool UIReloadUIBehaviour::requireStyleUpdate() const
{
	for (const auto& styleObserver : uiStyleObservers) {
		if (styleObserverNeedsUpdate(styleObserver)) {
			return true;
		}
	}

	return false;
}

bool UIReloadUIBehaviour::styleObserverNeedsUpdate(const std::pair<String, int>& styleObserver) const
{
	return factory.getStyleSheet()->getStyleObserver(styleObserver.first).getAssetVersion() != styleObserver.second;
}

void UIReloadUIBehaviour::setupUIStyleObservers()
{
	uiStyleObservers.clear();
	auto* widget = getWidget();
	const auto stylesheet = factory.getStyleSheet();
	getStyleObservers(*widget, *stylesheet, uiStyleObservers);
}

void UIReloadUIBehaviour::getStyleObservers(const UIWidget& widget, const UIStyleSheet& stylesheet, Vector<std::pair<String, int>>& styleObservers) const
{
	if (widget.hasStyle()) {
		for (const auto& style: widget.getStyles()) {
			if (stylesheet.hasStyleObserver(style.getName()) && !std_ex::contains_if(styleObservers, [&](const auto& so) { return so.first == style.getName(); })) {
				styleObservers.emplace_back(style.getName(), stylesheet.getStyleObserver(style.getName()).getAssetVersion());
			}
		}
	}

	for (const auto& child : widget.getChildren()) {
		getStyleObservers(*child, stylesheet, styleObservers);
	}

	for (const auto& child : widget.getChildrenWaiting()) {
		getStyleObservers(*child, stylesheet, styleObservers);
	}
}