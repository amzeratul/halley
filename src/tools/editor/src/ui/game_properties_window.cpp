#include "game_properties_window.h"
using namespace Halley;

GamePropertiesWindow::GamePropertiesWindow(UIFactory& factory)
	: UIWidget("game_properties_window", Vector2f(), UISizer())
{
	factory.loadUI(*this, "ui/halley/game_properties");
}

void GamePropertiesWindow::onMakeUI()
{
	
}
