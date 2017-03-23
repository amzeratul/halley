#pragma once

#include <algorithm>
#include <gsl/gsl_assert>
#include "family_type.h"
#include "family_mask.h"
#include "entity_id.h"
#include "halley/data_structures/nullable_reference.h"
#include "halley/support/exception.h"
#include "halley/support/debug.h"

namespace Halley {
	class Entity;
	class FamilyBindingBase;

	class Family {
		friend class World;

	public:
		Family(FamilyMaskType mask);
		virtual ~Family() {}

		size_t count() const
		{
			return elemCount;
		}

		void* getElement(size_t n) const
		{
			return static_cast<char*>(elems) + (n * elemSize);
		}

		void addOnEntitiesAdded(FamilyBindingBase* bind);
		void removeOnEntityAdded(FamilyBindingBase* bind);
		void addOnEntitiesRemoved(FamilyBindingBase* bind);
		void removeOnEntityRemoved(FamilyBindingBase* bind);

		void notifyAdd(void* entities, size_t count);
		void notifyRemove(void* entities, size_t count);

	protected:
		virtual void addEntity(Entity& entity) = 0;
		void removeEntity(Entity& entity);
		virtual void updateEntities() = 0;
		virtual void clearEntities() = 0;
		
		void* elems = nullptr;
		size_t elemCount = 0;
		size_t elemSize = 0;
		Vector<EntityId> toRemove;

		Vector<FamilyBindingBase*> addEntityCallbacks;
		Vector<FamilyBindingBase*> removeEntityCallbacks;

	private:
		FamilyMaskType inclusionMask;
	};

	class FamilyBase {
	protected:
		NullableReferenceAnchor anchor;

	public:
		EntityId entityId;
	};

	template <typename T>
	class FamilyBaseOf : public FamilyBase {
	public:
		NullableReferenceOf<T> getReference()
		{
			return anchor.getReferenceOf<T>();
		}

		NullableReferenceOf<const T> getReference() const
		{
			return anchor.getReferenceOf<const T>();
		}
	};
	
	// Apple's Clang 3.5 does not seem to have constexpr std::max...
	constexpr size_t maxSize(size_t a, size_t b)
	{
		return a > b ? a : b;
	}

	template <typename T>
	class FamilyImpl : public Family
	{
		struct StorageType : public FamilyBase
		{
			alignas(alignof(void*)) std::array<char, sizeof(T) - maxSize(sizeof(FamilyBase), 2 * sizeof(void*))> data;
		};
		static_assert(((T::Type::getNumComponents() + 2) * sizeof(void*)) == sizeof(T), "Family type has unexpected storage size");

	public:
		FamilyImpl() : Family(T::Type::inclusionMask()) {}
				
	protected:
		void addEntity(Entity& entity) override
		{
			entities.push_back(StorageType());
			auto& e = entities.back();
			e.entityId = entity.getEntityId();
			T::Type::loadComponents(entity, &e.data[0]);

			dirty = true;
		}

		void updateEntities() override
		{
			if (dirty) {
				// Notify additions
				HALLEY_DEBUG_TRACE();
				size_t prevSize = elemCount;
				size_t curSize = entities.size();
				updateElems();
				Expects(curSize >= prevSize);
				if (curSize > prevSize) {
					notifyAdd(entities.data() + prevSize, curSize - prevSize);
				}

				dirty = false;
			}

			// Remove
			removeDeadEntities();
		}

		void clearEntities() override
		{
			notifyRemove(entities.data(), entities.size());
			entities.clear();
			updateElems();
		}

	private:
		Vector<StorageType> entities;
		bool dirty = false;

		void updateElems()
		{
			elems = entities.empty() ? nullptr : entities.data();
			elemCount = entities.size();
			elemSize = sizeof(StorageType);
		}

		void removeDeadEntities()
		{
			// Performance-critical code
			// Benchmarks suggest that using a Vector is faster than std::set and std::unordered_set
			if (!toRemove.empty()) {
				HALLEY_DEBUG_TRACE();
				size_t removeCount = toRemove.size();
				Expects(removeCount > 0);
				Expects(removeCount <= entities.size());
				std::sort(toRemove.begin(), toRemove.end());

				for (size_t i = 1; i < toRemove.size(); ++i) {
					Expects(toRemove[i - 1] != toRemove[i]);
				}

				// Move all entities to be removed to the back of the vector
				{
					int n = int(entities.size());
					// Note: it's important to scan it forward. Scanning backwards would improve performance for short-lived entities,
					// but it causes an issue where an entity is removed and added to the same family in one frame.
					for (int i = 0; i < n; i++) {
						EntityId id = entities[i].entityId;
						auto iter = std::lower_bound(toRemove.begin(), toRemove.end(), id);
						if (iter != toRemove.end() && id == *iter) {
							toRemove.erase(iter);
							if (i != n - 1) {
								std::swap(entities[i], entities[n - 1]);
								i--;
							}
							n--;
							if (toRemove.empty()) {
								break;
							}
						}
					}
					Ensures(size_t(n) + removeCount == entities.size());
				}

				Expects(toRemove.empty());

				// Notify removal
				size_t newSize = entities.size() - removeCount;
				Ensures(newSize < entities.size());
				notifyRemove(entities.data() + newSize, removeCount);

				// Remove them
				entities.resize(newSize);
				updateElems();
			}
			Ensures(toRemove.empty());
		}
	};
}
