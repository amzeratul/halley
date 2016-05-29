#pragma once

#include "family_mask.h"
#include "world.h"

namespace Halley {
	class Family;

	class FamilyBindingBase {
	public:
		size_t count() const { return family->count(); }
		virtual ~FamilyBindingBase() = default;

	protected:
		FamilyBindingBase(FamilyMaskType readMask, FamilyMaskType writeMask);
		void* getElement(size_t index) const { return family->getElement(index); }
		virtual void bindFamily(World& world) = 0;
		void setFamily(Family* family);

	private:
		friend class System;

		Family* family = nullptr;
		const FamilyMaskType readMask;
		const FamilyMaskType writeMask;
	};

	template <typename T>
	class FamilyBinding : public FamilyBindingBase
	{
	public:
		FamilyBinding() : FamilyBindingBase(T::Type::readMask(), T::Type::writeMask()) {}

		T& operator[](size_t index) {
			return *reinterpret_cast<T*>(getElement(index));
		}
		
		const T& operator[](size_t index) const {
			return *reinterpret_cast<T*>(getElement(index));
		}

		T* begin()
		{
			return reinterpret_cast<T*>(getElement(0));
		}

		const T* begin() const
		{
			return reinterpret_cast<T*>(getElement(0));
		}

		T* end()
		{
			return reinterpret_cast<T*>(getElement(count()));
		}

		const T* end() const
		{
			return reinterpret_cast<T*>(getElement(count()));
		}

	protected:
		void bindFamily(World& world) override {
			setFamily(&world.getFamily<T>());
		}
	};
}
