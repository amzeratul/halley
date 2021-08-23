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

void UIBehaviour::onParentAboutToDraw()
{
}

bool UIBehaviour::isAlive() const
{
	return true;
}

UIWidget* UIBehaviour::getWidget() const
{
	return widget;
}

void UIBehaviour::setReversed(bool r)
{
	reversed = r;
}

bool UIBehaviour::isReversed() const
{
	return reversed;
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
