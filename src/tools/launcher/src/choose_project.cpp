#include "choose_project.h"
using namespace Halley;

ChooseProject::ChooseProject(UIFactory& factory)
	: UIWidget("choose_project", {}, UISizer())
	, factory(factory)
{
	factory.loadUI(*this, "ui/load_project");
}

void ChooseProject::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "open", [=] (const UIEvent& event)
	{
		FileChooserParameters parameters;
		parameters.folderOnly = true;
		OS::get().openFileChooser(parameters).then(Executors::getMainThread(), [] (std::optional<Path> path)
		{
			if (path) {
				Logger::logInfo("Selected path: " + path.value().toString());
			} else {
				Logger::logInfo("Cancelled path selection.");
			}
		});
	});
}
