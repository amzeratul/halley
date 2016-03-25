#pragma once

#include "family_extractor.h"

namespace Halley {
	template <typename... Ts>
	class FamilyType {
	public:
		constexpr static FamilyMaskType writeMask = FamilyMask::MutableEvaluator<Ts...>::getMask(0);
		constexpr static FamilyMaskType readMask = FamilyMask::Evaluator<Ts...>::getMask(0);

		static void loadComponents(Entity& entity, char* data)
		{
			Halley::FamilyExtractor::Evaluator<Ts...>::buildEntity(entity, reinterpret_cast<void**>(data), 0);
		}
	};
}
