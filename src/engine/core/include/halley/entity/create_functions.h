#pragma once

#include "registry.h"

namespace Halley {
	class CodegenFunctions;

	class CreateEntityFunctions {
	public:
		static std::unique_ptr<CodegenFunctions>& getCodegenFunctions();
	};
}
