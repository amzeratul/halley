#pragma once

#include "family_type.h"

namespace Halley {
	class Family;

	class FamilyBindingBase {
	public:
		size_t count() const;

	protected:
		FamilyBindingBase(FamilyMaskType readMask, FamilyMaskType writeMask);
		void* getElement(size_t index) const;

	private:
		friend class System;
		void setFamily(Family* family);

		Family* family;
		const FamilyMaskType readMask;
		const FamilyMaskType writeMask;
	};

	template <typename T>
	class FamilyBinding : public FamilyBindingBase {
	public:
		FamilyBinding() : FamilyBindingBase(T::Type::mask, T::Type::mask) {}

		T& operator[](size_t index) {
			return *reinterpret_cast<T*>(getElement(index));
		}
	};
}
