#include "halley/ui/ui_behaviour.h"
using namespace Halley;

UIBehaviour::~UIBehaviour() = default;

void UIBehaviour::init()
{
}

void UIBehaviour::deInit()
{
}

void UIBehaviour::update(Time time)
{
}

bool UIBehaviour::onParentDestroyed()
{
	return true;
}

bool UIBehaviour::isAlive() const
{
	return true;
}

UIWidget* UIBehaviour::getWidget() const
{
	return widget;
}

void UIBehaviour::doInit(UIWidget& w)
{
	widget = &w;
	init();
}

void UIBehaviour::doDeInit()
{
	deInit();
}
