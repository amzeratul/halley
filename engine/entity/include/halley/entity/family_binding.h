#pragma once

#include "family_mask.h"
#include "world.h"
#include <halley/support/exception.h>

namespace Halley {
	class Family;

	class FamilyBindingBase {
	public:
		size_t count() const { return family->count(); }
		size_t size() const { return family->count(); }
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
			Expects(index < count());
			return *reinterpret_cast<T*>(getElement(index));
		}
		
		const T& operator[](size_t index) const {
			Expects(index < count());
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

		T& getSingleton()
		{
			Expects(count() == 1);
			return *begin();
		}

		const T& getSingleton() const
		{
			Expects(count() == 1);
			return *begin();
		}

		T& operator()()
		{
			return getSingleton();
		}

		const T& operator()() const
		{
			return getSingleton();
		}

		template <typename F>
		T* tryMatch(F f)
		{
			for (auto& e: *this) {
				if (f(e)) {
					return &e;
				}
			}
			return nullptr;
		}

		template <typename F>
		const T* tryMatch(F f) const
		{
			for (auto& e: *this) {
				if (f(e)) {
					return &e;
				}
			}
			return nullptr;
		}

		template <typename F>
		T& match(F f)
		{
			auto res = tryMatch(f);
			if (res) return *res;
			throw Exception("No element in family matches predicate.");
		}

		template <typename F>
		const T& match(F f) const
		{
			auto res = tryMatch(f);
			if (res) return *res;
			throw Exception("No element in family matches predicate.");
		}

	protected:
		void bindFamily(World& world) override {
			setFamily(&world.getFamily<T>());
		}
	};
}
