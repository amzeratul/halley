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

		void doInit(FamilyMaskType readMask, FamilyMaskType writeMask, World& world) noexcept;
		
		void* getRawElement(size_t index) const noexcept { return family->getRawElement(index); }
		template <typename T>
		T* getElement(size_t index) const noexcept { return family->getElement<T>(index); }
		void setFamily(Family* family) noexcept;

		template <typename T>
		T* findElement(EntityId id) const noexcept { return family->findElement<T>(id); }

		void setIndexed();
		void setOnEntitiesAdded(std::function<void(void*, size_t)> callback);
		void setOnEntitiesRemoved(std::function<void(void*, size_t)> callback);
		void setOnEntitiesReloaded(std::function<void(void*, size_t)> callback);
		
		void onEntitiesAdded(void* entities, size_t count);
		void onEntitiesRemoved(void* entities, size_t count);
		void onEntitiesReloaded(void* entities, size_t count);

		Family& getFamily() const { return *family; }

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

	protected:
		World* world = nullptr;
	};

#if defined(DEV_BUILD) && !defined(NN_NINTENDO_SDK)
#define FAMILY_BINDING_DEBUG_ITERATORS
#endif

#if !defined(NN_NINTENDO_SDK)
#define FAMILY_BINDING_PREFETCH
#endif
	
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

		constexpr FamilyBindingIterator()
			: v(nullptr)
		{}
		constexpr FamilyBindingIterator(pointer v, uint32_t prefetchDist, uint32_t elemsLeft, World& world)
			: v(v)
#ifdef FAMILY_BINDING_PREFETCH
			, prefetchDist(prefetchDist)
			, elemsLeft(elemsLeft)
#endif
#ifdef FAMILY_BINDING_DEBUG_ITERATORS
			, familyRevision(world.getFamilyRevision())
			, world(&world)
#endif
		{}
		constexpr FamilyBindingIterator(const FamilyBindingIterator& o) = default;
		
		constexpr reference operator*() const
		{
#ifdef FAMILY_BINDING_DEBUG_ITERATORS
			if (familyRevision != world->getFamilyRevision()) {
				throw Exception("World family revision changed due to World::updateEntities(), this iterator has been invalidated", HalleyExceptions::Entity);
			}
#endif
			return *v;
		}

		constexpr pointer operator->() const
		{
#ifdef FAMILY_BINDING_DEBUG_ITERATORS
			if (familyRevision != world->getFamilyRevision()) {
				throw Exception("World family revision changed due to World::updateEntities(), this iterator has been invalidated", HalleyExceptions::Entity);
			}
#endif
			return v;
		}
		
		constexpr FamilyBindingIterator& operator++()
		{
#ifdef FAMILY_BINDING_DEBUG_ITERATORS
			if (familyRevision != world->getFamilyRevision()) {
				throw Exception("World family revision changed due to World::updateEntities(), this iterator has been invalidated", HalleyExceptions::Entity);
			}
#endif

			++v;

#ifdef FAMILY_BINDING_PREFETCH
			--elemsLeft;
			if (prefetchDist > 0 && elemsLeft > prefetchDist) {
				(v + prefetchDist)->prefetch();
			}
#endif

			return *this;
		}
		
		[[nodiscard]] constexpr bool operator==(const FamilyBindingIterator& other) const { return v == other.v; }
		[[nodiscard]] constexpr bool operator!=(const FamilyBindingIterator& other) const { return v != other.v; }

		constexpr friend void swap(FamilyBindingIterator& a, FamilyBindingIterator& b) noexcept { std::swap(a.v, b.v); }

	private:
		pointer v;
#ifdef FAMILY_BINDING_PREFETCH
		uint32_t prefetchDist = 1;
		uint32_t elemsLeft = 0;
#endif
#ifdef FAMILY_BINDING_DEBUG_ITERATORS
		uint32_t familyRevision = 0;
		World* world = nullptr;
#endif
	};

	template <typename T>
	class FamilyBinding : public FamilyBindingBase
	{
	public:

		constexpr FamilyBinding()
		{
			bindFamily = &bindFamilyImpl;
		}
		
		[[nodiscard]] constexpr T& operator[](size_t index) {
			HalleyAssertDebug(index < count());
			return *getFamilyElement(index);
		}
		
		[[nodiscard]] constexpr const T& operator[](size_t index) const {
			HalleyAssertDebug(index < count());
			return *getFamilyElement(index);
		}

		[[nodiscard]] constexpr FamilyBindingIterator<T> begin()
		{
#ifdef FAMILY_BINDING_DEBUG_ITERATORS
			if (getFamily().isDirty()) {
				throw Exception("Attempting to iterate through a family which is flagged as dirty. (Accessing other families is not allowed from onEntitiesAdded/etc context)", HalleyExceptions::Entity);
			}
#endif
			return FamilyBindingIterator(getFamilyElement(0), 5, static_cast<uint32_t>(count()), *world);
		}

		[[nodiscard]] constexpr FamilyBindingIterator<T> begin() const
		{
#ifdef FAMILY_BINDING_DEBUG_ITERATORS
			if (getFamily().isDirty()) {
				throw Exception("Attempting to iterate through a family which is flagged as dirty. (Accessing other families is not allowed from onEntitiesAdded/etc context)", HalleyExceptions::Entity);
			}
#endif
			return FamilyBindingIterator(getFamilyElement(0), 5, static_cast<uint32_t>(count()), *world);
		}

		[[nodiscard]] constexpr FamilyBindingIterator<T> end()
		{
			return FamilyBindingIterator(getFamilyElement(count()), 5, 0, *world);
		}

		[[nodiscard]] constexpr FamilyBindingIterator<T> end() const
		{
			return FamilyBindingIterator(getFamilyElement(count()), 5, 0, *world);
		}

		T& getSingleton()
		{
			if (count() != 1) {
				throw Exception(String("Attempting to access family of ") + typeid(T).name() + " as singleton, but it has " + toString(count()) + " elements.", HalleyExceptions::Entity);
			}
#ifdef FAMILY_BINDING_DEBUG_ITERATORS
			if (getFamily().isDirty()) {
				throw Exception("Attempting to access a family which is flagged as dirty. (Accessing other families is not allowed from onEntitiesAdded/etc context)", HalleyExceptions::Entity);
			}
#endif
			return *getFamilyElement(0);
		}

		const T& getSingleton() const
		{
			if (count() != 1) {
				throw Exception(String("Attempting to access family of ") + typeid(T).name() + " as singleton, but it has " + toString(count()) + " elements.", HalleyExceptions::Entity);
			}
#ifdef FAMILY_BINDING_DEBUG_ITERATORS
			if (getFamily().isDirty()) {
				throw Exception("Attempting to access a family which is flagged as dirty. (Accessing other families is not allowed from onEntitiesAdded/etc context)", HalleyExceptions::Entity);
			}
#endif
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
			return getFamily().template findElement<T>(id);
		}

		const T* tryFind(EntityId id) const
		{
			return getFamily().template findElement<T>(id);
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
		void init(World& world) noexcept
		{
			auto& storage = world.getMaskStorage();
			doInit(T::Type::readMask(storage), T::Type::writeMask(storage), world);
		}

		T* getFamilyElement(size_t i) const
		{
			return getElement<T>(i);
		}

		static void bindFamilyImpl(FamilyBindingBase& obj, World& world) noexcept
		{
			auto& self = static_cast<FamilyBinding&>(obj);
			self.init(world);
			self.setFamily(&world.getFamily<T>());
		}
	};
}
