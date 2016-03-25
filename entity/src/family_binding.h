#pragma once

#include "family_type.h"
#include "world.h"

namespace Halley {
	class Family;

	class FamilyBindingBase {
	public:
		size_t count() const;
		~FamilyBindingBase() = default;

	protected:
		FamilyBindingBase(FamilyMaskType readMask, FamilyMaskType writeMask);
		void* getElement(size_t index) const;
		virtual void bindFamily(World& world) = 0;
		void setFamily(Family* family);

	private:
		friend class System;

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

	protected:
		void bindFamily(World& world) override {
			setFamily(&world.getFamily<T>());
		}
	};
}
