#include <halley/entity/create_functions.h>
using namespace Halley;

std::unique_ptr<CodegenFunctions>& CreateEntityFunctions::getCodegenFunctions()
{
	static std::unique_ptr<CodegenFunctions> f;
	return f;
}
