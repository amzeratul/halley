#include "choose_project.h"
using namespace Halley;

ChooseProject::ChooseProject(UIFactory& factory)
	: UIWidget("choose_project", {}, UISizer())
	, factory(factory)
{
	factory.loadUI(*this, "ui/load_project");

	loadPaths();
}

void ChooseProject::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "open", [=] (const UIEvent& event)
	{
		FileChooserParameters parameters;
		parameters.folderOnly = true;
		OS::get().openFileChooser(parameters).then(Executors::getMainThread(), [=] (std::optional<Path> path)
		{
			if (path) {
				addNewPath(path.value());
			}
		});
	});
}

void ChooseProject::loadPaths()
{
	refresh();
}

void ChooseProject::savePaths()
{
	
}

void ChooseProject::addNewPath(Path path)
{
	addPath(path);
	savePaths();
	refresh();
}

void ChooseProject::addPath(Path path)
{
	paths.emplace_back(std::move(path));
}

void ChooseProject::refresh()
{
	
}
