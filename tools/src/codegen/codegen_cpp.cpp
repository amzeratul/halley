#include <sstream>
#include "codegen_cpp.h"
using namespace Halley;

static String toFileName(String className)
{
	std::stringstream ss;
	for (size_t i = 0; i < className.size(); i++) {
		if (className[i] >= 'A' && className[i] <= 'Z') {
			if (i > 0) {
				ss << '_';
			}
			ss << static_cast<char>(className[i] + 32);
		} else {
			ss << className[i];
		}
	}
	return ss.str();
}

CodeGenResult CodegenCPP::generateComponent(ComponentSchema component)
{
	String className = component.name;

	CodeGenResult result;
	result.emplace_back(CodeGenFile("components/" + toFileName(className) + ".h", { "hello world! :)" }));
	return result;
}

CodeGenResult CodegenCPP::generateSystem(SystemSchema system)
{
	String className = system.name;

	CodeGenResult result;
	result.emplace_back(CodeGenFile("systems/" + toFileName(className) + ".h", { "hello world!" }));
	return result;
}
