#pragma once

#include "family_mask.h"
#include "world.h"
#include <halley/support/exception.h>
#include <functional>

namespace Halley {
	class Family;

	class FamilyBindingBase {
	public:
		size_t count() const { return family->count(); }
		size_t size() const { return family->count(); }

		virtual ~FamilyBindingBase();

	protected:
		void doInit(FamilyMaskType readMask, FamilyMaskType writeMask);
		
		void* getElement(size_t index) const { return family->getElement(index); }
		virtual void bindFamily(World& world) = 0;
		void setFamily(Family* family);

		void setOnEntitiesAdded(std::function<void(void*, size_t)> callback);
		void setOnEntitiesRemoved(std::function<void(void*, size_t)> callback);

		void onEntitiesAdded(void* entities, size_t count);
		void onEntitiesRemoved(void* entities, size_t count);

	private:
		friend class System;
		friend class Family;

		Family* family = nullptr;
		FamilyMaskType readMask;
		FamilyMaskType writeMask;
		std::function<void(void*, size_t)> addedCallback;
		std::function<void(void*, size_t)> removedCallback;
	};

	template <typename T>
	class FamilyBinding : public FamilyBindingBase
	{
	public:
		void init(MaskStorage& storage)
		{
			doInit(T::Type::readMask(storage), T::Type::writeMask(storage));
		}

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
			throw Exception("No element in family matches predicate.", HalleyExceptions::Entity);
		}

		template <typename F>
		const T& match(F f) const
		{
			auto res = tryMatch(f);
			if (res) return *res;
			throw Exception("No element in family matches predicate.", HalleyExceptions::Entity);
		}

	protected:
		void bindFamily(World& world) override {
			init(world.getStorage());
			setFamily(&world.getFamily<T>());
		}
	};
}
