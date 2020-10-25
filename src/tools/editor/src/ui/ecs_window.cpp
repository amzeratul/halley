#include "ecs_window.h"
using namespace Halley;

ECSWindow::ECSWindow(UIFactory& factory)
	: UIWidget("ecs_window", Vector2f(), UISizer())
{
	factory.loadUI(*this, "ui/halley/ecs");
}

void ECSWindow::onMakeUI()
{
	
}
