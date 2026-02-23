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

void UIBehaviour::onAddedToRoot(UIRoot& root)
{
}

bool UIBehaviour::onParentDestroyRequested()
{
	return true;
}

void UIBehaviour::onParentDestroyed()
{
}

void UIBehaviour::onParentAboutToDraw()
{
}

void UIBehaviour::onActiveChanged(bool active)
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

void UIBehaviour::setInitial(bool initial)
{
	this->initial = initial;
}

bool UIBehaviour::isInitial() const
{
	return initial;
}

void UIBehaviour::restart()
{
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
