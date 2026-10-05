#pragma once
#include "entity.h"

namespace Halley {
	namespace FamilyExtractor
	{
		template <typename T>
		struct StripMaybeRef {
			using type = T;
			constexpr static bool isMaybe = false;
		};

		template <typename T>
		struct StripMaybeRef<MaybeRef<T>> {
			using type = T;
			constexpr static bool isMaybe = true;
		};


		template <typename... Ts>
		struct Evaluator;

		template <>
		struct Evaluator <> {
			static void buildEntity(Entity&, void**, size_t) {}
			static void buildEntityOptional(Entity&, void**, size_t) {}
		};

		template <typename T, typename... Ts>
		struct Evaluator <T, Ts...> {
			static void buildEntity(Entity& entity, void** data, size_t offset) {
				data[offset] = entity.tryGetComponent<typename StripMaybeRef<T>::type>(true);
				Evaluator<Ts...>::buildEntity(entity, data, offset + 1);
			}

			static void buildEntityOptional(Entity& entity, void** data, size_t offset) {
				if constexpr (StripMaybeRef<T>::isMaybe) {
					data[offset] = entity.tryGetComponent<typename StripMaybeRef<T>::type>(true);
				}
				Evaluator<Ts...>::buildEntityOptional(entity, data, offset + 1);
			}
		};
	}
}
