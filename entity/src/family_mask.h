#pragma once

namespace Halley {
	namespace FamilyMask {
		using Type = unsigned long long;

		constexpr static Type make(int index, Type mask = 0) {
			return (static_cast<Type>(1) << static_cast<Type>(index)) | mask;
		}

		template <typename... Ts>
		struct Evaluator;

		template <>
		struct Evaluator <> {
			static constexpr Type getMask(Type startValue) {
				return startValue;
			}
		};

		template <typename T, typename... Ts>
		struct Evaluator <T, Ts...> {
			static constexpr Type getMask(Type startValue) {
				return Evaluator<Ts...>::getMask(FamilyMask::make(T::componentIndex, startValue));
			}
		};

		template <typename... Ts>
		struct MutableEvaluator;

		template <>
		struct MutableEvaluator <> {
			static constexpr Type getMask(Type startValue) {
				return startValue;
			}
		};

		template <typename T, typename... Ts>
		struct MutableEvaluator <T, Ts...> {
			static constexpr Type getMask(Type startValue) {
				return Evaluator<Ts...>::getMask(FamilyMask::make(std::is_const<T>::value ? 0 : T::componentIndex, startValue));
			}
		};
	};

	using FamilyMaskType = FamilyMask::Type;
}
