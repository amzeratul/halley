#include "localisation_set_filters_window.h"

using namespace Halley;

LocalisationFiltersController::LocalisationFiltersController(UIWidget& ui, LocalisationFilters& filters)
	: ui(ui)
	, filters(filters)
{
}

void LocalisationFiltersController::setup()
{
	ui.bindData("priorityEnable", filters.priorityEnabled, [=] (bool value) {
		filters.priorityEnabled = value;
		setPriorityEnabled(value);
	});

	ui.bindData("readyStatusEnable", filters.readyEnabled, [=] (bool value) {
		filters.readyEnabled = value;
		setReadyEnabled(value);
	});

	ui.bindData("translationStatusEnable", filters.translatedEnabled, [=] (bool value) {
		filters.translatedEnabled = value;
		setTranslatedEnabled(value);
	});

	ui.bindData("outdatedStatusEnable", filters.outdatedEnabled, [=] (bool value) {
		filters.outdatedEnabled = value;
		setOutdatedEnabled(value);
	});

	ui.bindData("commentStatusEnable", filters.commentEnabled, [=] (bool value) {
		filters.commentEnabled = value;
		setCommentEnabled(value);
	});

	ui.bindData("contextStatusEnable", filters.contextEnabled, [=] (bool value) {
		filters.contextEnabled = value;
		setContextEnabled(value);
	});

	ui.bindData("minPriority", toString(filters.minPriority), [=] (String value) {
		filters.minPriority = fromString<LocPriority>(value);
	});

	ui.bindData("maxPriority", toString(filters.maxPriority), [=] (String value) {
		filters.maxPriority = fromString<LocPriority>(value);
	});

	ui.bindData("readyStatus", toString(filters.ready), [=] (String value) {
		filters.ready = fromString<LocReadyStatus>(value);
	});

	ui.bindData("translationStatus", toString(filters.translated), [=] (String value) {
		filters.translated = fromString<LocTranslatedStatus>(value);
	});

	ui.bindData("outdatedStatus", toString(filters.outdated), [=] (String value) {
		filters.outdated = fromString<LocOutdatedStatus>(value);
	});

	ui.bindData("commentStatus", toString(filters.comment), [=] (String value) {
		filters.comment = fromString<LocCommentStatus>(value);
	});

	ui.bindData("contextStatus", toString(filters.context), [=] (String value) {
		filters.context = fromString<LocContextStatus>(value);
	});

	setPriorityEnabled(filters.priorityEnabled);
	setReadyEnabled(filters.readyEnabled);	
	setTranslatedEnabled(filters.translatedEnabled);
	setOutdatedEnabled(filters.outdatedEnabled);	
	setCommentEnabled(filters.commentEnabled);	
	setContextEnabled(filters.contextEnabled);	
}

void LocalisationFiltersController::setPriorityEnabled(bool enabled)
{
	ui.getWidget("priorityFields")->setActive(enabled);
}

void LocalisationFiltersController::setTranslatedEnabled(bool enabled)
{
	ui.getWidget("translationStatus")->setActive(enabled);
}

void LocalisationFiltersController::setOutdatedEnabled(bool enabled)
{
	ui.getWidget("outdatedStatus")->setActive(enabled);
}

void LocalisationFiltersController::setReadyEnabled(bool enabled)
{
	ui.getWidget("readyStatus")->setActive(enabled);
}

void LocalisationFiltersController::setCommentEnabled(bool enabled)
{
	ui.getWidget("commentStatus")->setActive(enabled);
}

void LocalisationFiltersController::setContextEnabled(bool enabled)
{
	ui.getWidget("contextStatus")->setActive(enabled);
}


LocalisationSetFiltersWindow::LocalisationSetFiltersWindow(UIFactory& factory, LocalisationFilters& filters, Vector2f pos, Callback callback)
	: UIWidget("set_filters", {}, UISizer())
	, factory(factory)
	, originalFilters(filters)
	, workingCopy(filters)
	, callback(std::move(callback))
	, filterController(*this, workingCopy)
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

	filterController.setup();
}

void LocalisationSetFiltersWindow::applyFilters()
{
	originalFilters = workingCopy;
}
