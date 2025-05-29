#include "localisation_export_window.h"

using namespace Halley;

LocalisationExportWindow::LocalisationExportWindow(UIFactory& factory, Callback callback)
	: UIWidget("localisation_export_window", {}, UISizer())
	, factory(factory)
	, callback(callback)
	, filterController(*this, filters)
{
	filters.initialise(true);

	factory.loadUI(*this, "halley/localisation/localisation_export");
	UIWidget::setAnchor(UIAnchor());
}

void LocalisationExportWindow::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "export", [=] (const UIEvent& event) {
		callback(true, filters);
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=] (const UIEvent& event) {
		callback(false, {});
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "groupSelectAll", [=] (const UIEvent& event) {
		// TODO
	});

	setHandle(UIEventType::ButtonClicked, "groupSelectNone", [=] (const UIEvent& event) {
		// TODO
	});

	filterController.setup();
}
