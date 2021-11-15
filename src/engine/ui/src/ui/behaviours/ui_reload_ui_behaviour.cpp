#include "halley/ui/behaviours/ui_reload_ui_behaviour.h"

#include "ui_factory.h"
#include "ui_widget.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/support/logger.h"
#include "ui_definition.h"

using namespace Halley;

UIReloadUIBehaviour::UIReloadUIBehaviour(UIFactory& factory, ResourceObserver observer)
	: factory(factory)
	, observer(observer)
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
			auto ui = factory.makeUIFromNode(dynamic_cast<const UIDefinition*>(observer.getResourceBeingObserved())->getRoot());

			// The above might throw, don't clear until after we know it hasn't
			getWidget()->clear();
			getWidget()->add(std::move(ui), 1);
			getWidget()->onMakeUI();

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

void UIReloadUIBehaviour::getStyleObservers(const UIWidget& widget, const UIStyleSheet& stylesheet, std::vector<std::pair<String, int>>& styleObservers) const
{
	if(widget.hasStyle()) {
		for(const auto& style : widget.getStyles()) {
			if (stylesheet.hasStyleObserver(style.getName())) {
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