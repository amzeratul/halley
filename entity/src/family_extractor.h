#pragma once
#include "entity.h"

namespace Halley {
	namespace FamilyExtractor
	{
		template <typename... Ts>
		struct Evaluator;

		template <>
		struct Evaluator <> {
			static void buildEntity(Entity&, void**, size_t) {}
		};

		template <typename T, typename... Ts>
		struct Evaluator <T, Ts...> {
			static void buildEntity(Entity& entity, void** data, size_t offset) {
				data[offset] = entity.getComponent<T>();
				Evaluator<Ts...>::buildEntity(entity, data, offset + 1);
			}
		};
	}
}
