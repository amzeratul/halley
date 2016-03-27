#include "stage.h"

using namespace Halley;

Stage::Stage(String _name)
	: name(_name)
{
}

void Stage::doInit(HalleyAPI* _api)
{
	api = _api;
	init();
}

void Stage::doDeInit()
{
	deInit();
}
