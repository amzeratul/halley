#pragma once

#include <memory>
#include "ecs_reflection.h"
#include "ecs_reflection_impl.h"

namespace Halley {
	class CodegenFunctions {
	public:
		virtual ~CodegenFunctions() = default;

		virtual Vector<SystemReflector> makeSystemReflectors() = 0;
		virtual Vector<std::unique_ptr<ComponentReflector>> makeComponentReflectors() = 0;
		virtual Vector<std::unique_ptr<MessageReflector>> makeMessageReflectors() = 0;
		virtual Vector<std::unique_ptr<SystemMessageReflector>> makeSystemMessageReflectors() = 0;
	};

	std::unique_ptr<CodegenFunctions> createCodegenFunctions();
}
