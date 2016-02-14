#pragma once

class FamilyMask {
public:
	using Type = long long;

	constexpr static Type Make(int index, Type mask = 0) {
		return (1ul << static_cast<Type>(index)) | mask;
	}
};
using FamilyMaskType = FamilyMask::Type;

template <typename... Ts>
class FamilyType {
	constexpr static FamilyMaskType getMask(FamilyMaskType startValue = 0) {
		//return FamilyMask::Make(T::componentIndex, startValue);
		return startValue;
	}
};

template <typename T, typename... Ts>
class FamilyType {
public:
	constexpr static FamilyMaskType getMask(FamilyMaskType startValue = 0) {
		return FamilyType<Ts>::getMask(FamilyMask::Make(T::componentIndex, startValue));
	}
};
