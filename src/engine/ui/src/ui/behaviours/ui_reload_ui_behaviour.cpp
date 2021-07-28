#include "halley/ui/behaviours/ui_reload_ui_behaviour.h"

#include "ui_factory.h"
#include "ui_widget.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/support/logger.h"

using namespace Halley;

UIReloadUIBehaviour::UIReloadUIBehaviour(UIFactory& factory, ConfigObserver observer)
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
			auto ui = factory.makeUIFromNode(observer.getRoot());

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
	std::vector<std::shared_ptr<UILabel>> labels;

	// TODO: Sort this out! Can this be a shared_ptr?
	uiStyleObservers.clear();
	auto* widget = getWidget();
	if(widget->hasStyle()) {
		uiStyleObservers.emplace_back(widget->getStyleName(), factory.getStyleSheet()->getStyleObserver(widget->getStyleName()).getAssetVersion());
	}
	
	for (const auto& child : widget->getChildren()) {
		getStyleObservers(child, uiStyleObservers);
	}

	for (const auto& child : widget->getChildrenWaiting()) {
		getStyleObservers(child, uiStyleObservers);
	}
	// TODO END
}

void UIReloadUIBehaviour::getStyleObservers(const std::shared_ptr<UIWidget>& widget, std::vector<std::pair<String, int>>& styleObservers) const
{
	if(widget->hasStyle()) {
		styleObservers.emplace_back(widget->getStyleName(), factory.getStyleSheet()->getStyleObserver(widget->getStyleName()).getAssetVersion());
	}

	for (const auto& child : widget->getChildren()) {
		getStyleObservers(child, styleObservers);
	}

	for (const auto& child : widget->getChildrenWaiting()) {
		getStyleObservers(child, styleObservers);
	}
}