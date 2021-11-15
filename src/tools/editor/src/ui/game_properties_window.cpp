#include "game_properties_window.h"
using namespace Halley;

GamePropertiesWindow::GamePropertiesWindow(UIFactory& factory, Project& project)
	: UIWidget("game_properties_window", Vector2f(), UISizer())
	, project(project)
{
	factory.loadUI(*this, "halley/game_properties");
}

void GamePropertiesWindow::onMakeUI()
{
	
}
