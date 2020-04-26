#include <halley/entity/create_functions.h>
using namespace Halley;

CreateComponentFunction& CreateEntityFunctions::getCreateComponent()
{
	static CreateComponentFunction f;
	return f;
}

CreateSystemFunction& CreateEntityFunctions::getCreateSystem()
{
	static CreateSystemFunction f;
	return f;
}
