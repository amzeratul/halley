#pragma once

#include <algorithm>
#include <gsl/gsl_assert>
#include "family_type.h"
#include "family_mask.h"
#include "entity_id.h"
#include "halley/data_structures/nullable_reference.h"
#include "halley/support/exception.h"
#include "halley/support/debug.h"
#include "halley/utils/utils.h"

namespace Halley {
	class Entity;
	class FamilyBindingBase;

	class Family {
		friend class World;

	public:
		explicit Family(FamilyMaskType inclusionMask, FamilyMaskType optionalMask);
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
		void addOnEntitiesReloaded(FamilyBindingBase* bind);
		void removeOnEntitiesReloaded(FamilyBindingBase* bind);

		void notifyAdd(void* entities, size_t count);
		void notifyRemove(void* entities, size_t count);
		void notifyReload(void* entities, size_t count);

	protected:
		virtual void addEntity(Entity& entity) = 0;
		virtual void refreshEntity(Entity& entity) = 0;
		void removeEntity(Entity& entity);
		void reloadEntity(Entity& entity);
		virtual void updateEntities() = 0;
		virtual void clearEntities() = 0;
		
		void* elems = nullptr;
		size_t elemCount = 0;
		size_t elemSize = 0;
		Vector<EntityId> toRemove;
		Vector<EntityId> toReload;

		Vector<FamilyBindingBase*> addEntityCallbacks;
		Vector<FamilyBindingBase*> removeEntityCallbacks;
		Vector<FamilyBindingBase*> modifiedEntityCallbacks;

	private:
		FamilyMaskType inclusionMask;
		FamilyMaskType optionalMask;
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
		constexpr static size_t storageSize = sizeof(T) - alignUp(sizeof(FamilyBase), alignof(void*));
		static_assert(std::is_base_of<FamilyBase, T>::value, "Family type does not derive from FamilyBase");

		// I don't know why this needs to be aligned up to 8 on Win32. :|
		static_assert(alignUp(T::Type::getNumComponents() * sizeof(void*), size_t(8)) == storageSize, "Family type has unexpected storage size");

		struct StorageType : public FamilyBase
		{
			alignas(alignof(void*)) std::array<char, storageSize> data;
		};

	public:
		explicit FamilyImpl(MaskStorage& storage)
			: Family(T::Type::inclusionMask(storage), T::Type::optionalMask(storage))
		{
		}
				
	protected:
		void addEntity(Entity& entity) override
		{
			auto& e = entities.emplace_back();
			e.entityId = entity.getEntityId();
			T::Type::loadComponents(entity, &e.data[0]);

			dirty = true;
		}
		
		void refreshEntity(Entity& entity) override
		{
			for (auto& e: entities) {
				if (e.entityId == entity.getEntityId()) {
					T::Type::loadComponents(entity, &e.data[0]);
					break;
				}
			}
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

			if (!toReload.empty()) {
				// Notify reloads
				HALLEY_DEBUG_TRACE();
				std::vector<StorageType*> reloadedEntities;
				for (auto& entity : entities) {
					if (std::find(toReload.begin(), toReload.end(), entity.entityId) != toReload.end()) {
						reloadedEntities.push_back(&entity);
					}
				}
				notifyReload(reloadedEntities.data(), reloadedEntities.size());
				toReload.clear();
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
