#pragma once

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
};

using FamilyMaskType = FamilyMask::Type;

template <typename... Ts>
class FamilyType {
public:
	constexpr static FamilyMaskType mask = FamilyMask::Evaluator<Ts...>::getMask(0);
};
