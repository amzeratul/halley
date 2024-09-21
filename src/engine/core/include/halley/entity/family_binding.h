#pragma once

#include "family_mask.h"
#include "world.h"
#include <halley/support/exception.h>
#include <functional>

namespace Halley {
	class Family;

	// This class is not virtual to avoid RTTI bloat, hence the function pointer to do a virtual method call
	class FamilyBindingBase {
	public:
		size_t count() const { return family->count(); }
		size_t size() const { return family->count(); }

		~FamilyBindingBase();

	protected:
		using BindFamilyCallback = void (*)(FamilyBindingBase&, World&) noexcept;

		void doInit(FamilyMaskType readMask, FamilyMaskType writeMask) noexcept;
		
		void* getElement(size_t index) const noexcept { return family->getElement(index); }
		void setFamily(Family* family) noexcept;

		void setOnEntitiesAdded(std::function<void(void*, size_t)> callback);
		void setOnEntitiesRemoved(std::function<void(void*, size_t)> callback);
		void setOnEntitiesReloaded(std::function<void(void*, size_t)> callback);
		
		void onEntitiesAdded(void* entities, size_t count);
		void onEntitiesRemoved(void* entities, size_t count);
		void onEntitiesReloaded(void* entities, size_t count);

		BindFamilyCallback bindFamily = nullptr;

	private:
		friend class System;
		friend class Family;

		Family* family = nullptr;
		FamilyMaskType readMask;
		FamilyMaskType writeMask;
		std::function<void(void*, size_t)> addedCallback;
		std::function<void(void*, size_t)> removedCallback;
		std::function<void(void*, size_t)> reloadedCallback;
	};

	
	template <typename T>
	class FamilyBindingIterator {
	public:
	#ifdef __cpp_lib_concepts
	    using iterator_concept = std::contiguous_iterator_tag;
	#endif // __cpp_lib_concepts
	    using iterator_category = std::forward_iterator_tag;
	    using value_type        = T;
	    using difference_type   = std::ptrdiff_t;
	    using pointer           = T*;
	    using reference         = T&;

		FamilyBindingIterator() : v(nullptr) {}
		FamilyBindingIterator(pointer v, uint32_t prefetchDist, uint32_t elemsLeft) : v(v), prefetchDist(prefetchDist), elemsLeft(elemsLeft) {}
		FamilyBindingIterator(const FamilyBindingIterator& o) = default;
		
		reference operator*() const { return *v; }
		pointer operator->() const { return v; }
		
		FamilyBindingIterator& operator++()
		{
			assert(elemsLeft > 0);
			++v;
			--elemsLeft;
			if (prefetchDist > 0 && elemsLeft >= prefetchDist) {
				//(v + prefetchDist)->prefetch();
			}
			return *this;
		}
		
		bool operator==(const FamilyBindingIterator& other) const { return v == other.v; }
		bool operator!=(const FamilyBindingIterator& other) const { return v != other.v; }

		friend void swap(FamilyBindingIterator& a, FamilyBindingIterator& b) noexcept { std::swap(a.v, b.v); }

	private:
		pointer v;
		uint32_t prefetchDist = 1;
		uint32_t elemsLeft = 0;
	};

	template <typename T>
	class FamilyBinding : public FamilyBindingBase
	{
	public:

		FamilyBinding()
		{
			bindFamily = &bindFamilyImpl;
		}
		
		T& operator[](size_t index) {
			assert(index < count());
			return *getFamilyElement(index);
		}
		
		const T& operator[](size_t index) const {
			assert(index < count());
			return *getFamilyElement(index);
		}

		FamilyBindingIterator<T> begin()
		{
			return FamilyBindingIterator(getFamilyElement(0), 5, static_cast<uint32_t>(count()));
		}

		FamilyBindingIterator<T> begin() const
		{
			return FamilyBindingIterator(getFamilyElement(0), 5, static_cast<uint32_t>(count()));
		}

		FamilyBindingIterator<T> end()
		{
			return FamilyBindingIterator(getFamilyElement(count()), 5, 0);
		}

		FamilyBindingIterator<T> end() const
		{
			return FamilyBindingIterator(getFamilyElement(count()), 5, 0);
		}

		T& getSingleton()
		{
			assert(count() == 1);
			return *getFamilyElement(0);
		}

		const T& getSingleton() const
		{
			assert(count() == 1);
			return *getFamilyElement(0);
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

		T* tryFind(EntityId id)
		{
			for (auto& e: *this) {
				if (e.entityId == id) {
					return &e;
				}
			}
			return nullptr;
		}

		const T* tryFind(EntityId id) const
		{
			for (auto& e: *this) {
				if (e.entityId == id) {
					return &e;
				}
			}
			return nullptr;
		}

		T& find(EntityId id)
		{
			auto res = tryFind(id);
			if (res) return *res;
			throw Exception("No element in family matches id.", HalleyExceptions::Entity);
		}

		const T& find(EntityId id) const
		{
			auto res = tryFind(id);
			if (res) return *res;
			throw Exception("No element in family matches id.", HalleyExceptions::Entity);
		}

		gsl::span<T> getSpan()
		{
			return gsl::span<T>(getFamilyElement(0), count());
		}

		gsl::span<const T> getSpan() const
		{
			return gsl::span<const T>(getFamilyElement(0), count());
		}

	private:
		void init(MaskStorage& storage) noexcept
		{
			doInit(T::Type::readMask(storage), T::Type::writeMask(storage));
		}

		T* getFamilyElement(size_t i) const
		{
			// WARNING: Strict aliasing rules violation
			return reinterpret_cast<T*>(getElement(i));
		}

		static void bindFamilyImpl(FamilyBindingBase& obj, World& world) noexcept
		{
			auto& self = static_cast<FamilyBinding&>(obj);
			self.init(world.getMaskStorage());
			self.setFamily(&world.getFamily<T>());
		}
	};
}
