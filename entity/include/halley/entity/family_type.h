#pragma once

#include "family_extractor.h"

namespace Halley {
	template <typename... Ts>
	class FamilyType {
	public:
		static FamilyMaskType writeMask() {
			return FamilyMask::MutableEvaluator<Ts...>::getMask();
		}

		static FamilyMaskType readMask() {
			return FamilyMask::Evaluator<Ts...>::getMask();
		}

		static void loadComponents(Entity& entity, char* data) {
			Halley::FamilyExtractor::Evaluator<Ts...>::buildEntity(entity, reinterpret_cast<void**>(data), 0);
		}
	};
}
