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
	for (auto& styleObserver : uiStyleObservers) {
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
	auto* widget = getWidget();
	for (const auto& child : widget->getChildren()) {
		getLabels(child, labels);
	}

	for (const auto& child : widget->getChildrenWaiting()) {
		getLabels(child, labels);
	}
	// TODO END

	uiStyleObservers.clear();
	for(const auto& label : labels) {
		const auto& styleName = label->getStyle().getName();
		uiStyleObservers.emplace_back(styleName, factory.getStyleSheet()->getStyleObserver(styleName).getAssetVersion());
	}
}

// TODO: Refactor this
void UIReloadUIBehaviour::getLabels(const std::shared_ptr<UIWidget>& widget, std::vector<std::shared_ptr<UILabel>>& labels) const
{
	auto label = std::dynamic_pointer_cast<UILabel>(widget);
	if (label) {
		labels.emplace_back(label);
	}

	for (const auto& child : widget->getChildren()) {
		getLabels(child, labels);
	}

	for (const auto& child : widget->getChildrenWaiting()) {
		getLabels(child, labels);
	}
}