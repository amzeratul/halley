#pragma once

#include <algorithm>
#include "family_type.h"
#include "family_mask.h"
#include "entity_id.h"

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

		void addOnEntityAdded(FamilyBindingBase* bind);
		void removeOnEntityAdded(FamilyBindingBase* bind);
		void addOnEntityRemoved(FamilyBindingBase* bind);
		void removeOnEntityRemoved(FamilyBindingBase* bind);

		void notifyAdd(void* entity);
		void notifyRemove(void* entity);

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

	// Apple's Clang 3.5 does not seem to have constexpr std::max...
	constexpr size_t maxSize(size_t a, size_t b)
	{
		return a > b ? a : b;
	}

	template <typename T>
	class FamilyImpl : public Family
	{
		struct StorageType
		{
			EntityId entityId;
			alignas(alignof(void*)) std::array<char, sizeof(T) - maxSize(sizeof(EntityId), sizeof(void*))> data;
		};
		static_assert(((T::Type::getNumComponents() + 1) * sizeof(void*)) == sizeof(T), "Family type has unexpected storage size");

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
				for (size_t i = prevSize; i < curSize; ++i) {
					notifyAdd(&entities[i]);
				}

				dirty = false;
			}

			// Remove
			removeDeadEntities();
		}

		void clearEntities() override
		{
			for (auto& e: entities) {
				notifyRemove(&e);
			}
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
				std::vector<size_t> removeIdx(toRemove.size());
				std::sort(toRemove.begin(), toRemove.end());

				// Find the ids
				{
					size_t j = 0;
					for (int i = int(entities.size()); --i >= 0;) {
						EntityId id = entities[i].entityId;
						auto iter = std::lower_bound(toRemove.begin(), toRemove.end(), id);
						if (iter != toRemove.end() && id == *iter) {
							toRemove.erase(iter);
							removeIdx[j++] = size_t(i);
							if (toRemove.empty()) {
								break;
							}
						}
					}
				}

				// Notify
				for (size_t i = 0; i < removeIdx.size(); ++i) {
					notifyRemove(&entities[removeIdx[i]]);
				}

				// Remove them
				size_t swapWith = entities.size();
				for (size_t i = 0; i < removeIdx.size(); ++i) {
					size_t idx = removeIdx[i];
					std::swap(entities[idx], entities[--swapWith]);
				}
				entities.resize(entities.size() - removeIdx.size());

				updateElems();
			}
			Ensures(toRemove.empty());
		}
	};
}
