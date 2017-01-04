#pragma once

#include <algorithm>
#include "family_type.h"
#include "family_mask.h"
#include "entity_id.h"
#include "halley/data_structures/nullable_reference.h"

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

	class FamilyBase : public NullableReferenceAnchor {
	public:
		EntityId entityId;
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
				size_t prevSize = elemCount;
				size_t curSize = entities.size();
				updateElems();
				notifyAdd(entities.data() + prevSize, curSize - prevSize);

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
				size_t removeCount = toRemove.size();
				std::sort(toRemove.begin(), toRemove.end());

				// Move all entities to be removed to the back of the vector
				{
					size_t swapWith = entities.size();
					for (int i = int(entities.size()); --i >= 0;) {
						EntityId id = entities[i].entityId;
						auto iter = std::lower_bound(toRemove.begin(), toRemove.end(), id);
						if (iter != toRemove.end() && id == *iter) {
							toRemove.erase(iter);
							std::swap(entities[i], entities[--swapWith]);
							if (toRemove.empty()) {
								break;
							}
						}
					}
				}

				// Notify removal
				size_t newSize = entities.size() - removeCount;
				notifyRemove(entities.data() + newSize, removeCount);

				// Remove them
				entities.resize(newSize);
				updateElems();
			}
			Ensures(toRemove.empty());
		}
	};
}
