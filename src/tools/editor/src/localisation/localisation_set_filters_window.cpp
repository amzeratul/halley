#include "localisation_set_filters_window.h"

using namespace Halley;

LocalisationSetFiltersWindow::LocalisationSetFiltersWindow(UIFactory& factory, LocalisationFilters& filters, Vector2f pos, Callback callback)
	: UIWidget("set_filters", {}, UISizer())
	, factory(factory)
	, originalFilters(filters)
	, workingCopy(filters)
	, callback(std::move(callback))
{
	factory.loadUI(*this, "halley/localisation/localisation_filters");
	UIWidget::setAnchor(UIAnchor(Vector2f(), Vector2f(), pos));
}

void LocalisationSetFiltersWindow::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "set", [=] (const UIEvent& event) {
		applyFilters();
		callback(true);
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=] (const UIEvent& event) {
		callback(false);
		destroy();
	});

	bindData("priorityEnable", workingCopy.priorityEnabled, [=] (bool value) {
		workingCopy.priorityEnabled = value;
		setPriorityEnabled(value);
	});

	bindData("translationStatusEnable", workingCopy.translatedEnabled, [=] (bool value) {
		workingCopy.translatedEnabled = value;
		setTranslatedEnabled(value);
	});

	bindData("outdatedStatusEnable", workingCopy.outdatedEnabled, [=] (bool value) {
		workingCopy.outdatedEnabled = value;
		setOutdatedEnabled(value);
	});

	bindData("minPriority", toString(workingCopy.minPriority), [=] (String value) {
		workingCopy.minPriority = fromString<LocPriority>(value);
	});

	bindData("maxPriority", toString(workingCopy.maxPriority), [=] (String value) {
		workingCopy.maxPriority = fromString<LocPriority>(value);
	});

	bindData("translationStatus", toString(workingCopy.translated), [=] (String value) {
		workingCopy.translated = fromString<LocTranslatedStatus>(value);
	});

	bindData("outdatedStatus", toString(workingCopy.outdated), [=] (String value) {
		workingCopy.outdated = fromString<LocOutdatedStatus>(value);
	});

	setPriorityEnabled(workingCopy.priorityEnabled);
	setTranslatedEnabled(workingCopy.translatedEnabled);
	setOutdatedEnabled(workingCopy.outdatedEnabled);
}

void LocalisationSetFiltersWindow::setPriorityEnabled(bool enabled)
{
	getWidget("minPriority")->setEnabled(enabled);
	getWidget("maxPriority")->setEnabled(enabled);
}

void LocalisationSetFiltersWindow::setTranslatedEnabled(bool enabled)
{
	getWidget("translationStatus")->setEnabled(enabled);
}

void LocalisationSetFiltersWindow::setOutdatedEnabled(bool enabled)
{
	getWidget("outdatedStatus")->setEnabled(enabled);
}

void LocalisationSetFiltersWindow::applyFilters()
{
	originalFilters = workingCopy;
}
