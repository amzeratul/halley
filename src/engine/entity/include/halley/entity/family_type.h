#pragma once

#include "family_extractor.h"

namespace Halley {
	template <typename... Ts>
	class FamilyType {
	public:
		constexpr static FamilyMaskType writeMask(MaskStorage& storage)
		{
			return FamilyMask::MutableEvaluator<Ts...>::getMask(storage);
		}

		constexpr static FamilyMaskType readMask(MaskStorage& storage)
		{
			return FamilyMask::Evaluator<Ts...>::getMask(storage);
		}

		constexpr static FamilyMaskType inclusionMask(MaskStorage& storage)
		{
			return FamilyMask::InclusionEvaluator<Ts...>::getMask(storage);
		}

		constexpr static FamilyMaskType optionalMask(MaskStorage& storage)
		{
			return FamilyMask::OptionalEvaluator<Ts...>::getMask(storage);
		}

		static void loadComponents(Entity& entity, char* data) {
			Halley::FamilyExtractor::Evaluator<Ts...>::buildEntity(entity, reinterpret_cast<void**>(data), 0);
		}

		constexpr static size_t getNumComponents()
		{
			return sizeof...(Ts);
		}
	};
}
