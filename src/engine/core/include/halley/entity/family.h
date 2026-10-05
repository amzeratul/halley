#pragma once

#include <algorithm>
#include "halley/support/assert.h"
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
		explicit Family(FamilyMaskType inclusionMask, std::optional<FamilyMaskType> exclusionMask, FamilyMaskType optionalMask);
		virtual ~Family() {}

		[[nodiscard]] constexpr size_t count() const
		{
			return elemCount;
		}

		template<typename T>
		[[nodiscard]] constexpr T* getElement(size_t n) const
		{
			return static_cast<T*>(elems) + n;
		}

		[[nodiscard]] constexpr void* getRawElement(size_t n) const
		{
			return static_cast<char*>(elems) + (n * elemSize);
		}

		template<typename T>
		[[nodiscard]] T* findElement(EntityId id) const
		{
			if (indexed) {
				if (auto idx = findElementInIndex(id)) [[likely]] {
					return getElement<T>(*idx);
				}
			} else {
				for (size_t i = 0; i < elemCount; ++i) {
					auto* e = getElement<T>(i);
					if (e->entityId == id) {
						return e;
					}
				}
			}
			return nullptr;
		}

		bool matches(const FamilyMaskType& entityMask, MaskStorage& storage) const {
			return entityMask.contains(inclusionMask, storage) && (!exclusionMask || !entityMask.intersects(*exclusionMask, storage));
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

		void setIndexed();
		virtual void rebuildIndex() = 0;

		virtual bool isDirty() const = 0;

	protected:
		virtual void addEntity(Entity& entity) = 0;
		virtual void refreshEntity(Entity& entity) = 0;
		virtual void refreshEntityOptionals(Entity& entity) = 0;
		void removeEntity(Entity& entity);
		void reloadEntity(Entity& entity);
		virtual void updateEntities() = 0;
		virtual void clearEntities() = 0;

		OptionalLite<size_t> findElementInIndex(EntityId id) const;
		
		void* elems = nullptr;
		size_t elemCount = 0;
		uint32_t elemSize = 0;
		bool indexed = false;
		
	private:
		FamilyMaskType inclusionMask;
		std::optional<FamilyMaskType> exclusionMask;
		FamilyMaskType optionalMask;

	protected:
		Vector<EntityId> toRemove;
		Vector<EntityId> toReload;

		Vector<FamilyBindingBase*> addEntityCallbacks;
		Vector<FamilyBindingBase*> removeEntityCallbacks;
		Vector<FamilyBindingBase*> modifiedEntityCallbacks;

		HashMap<EntityId, size_t> index;
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
	class FamilyImpl final : public Family
	{
		constexpr static size_t storageSize = sizeof(T) - alignUp(sizeof(FamilyBase), alignof(void*));
		static_assert(std::is_base_of<FamilyBase, T>::value, "Family type does not derive from FamilyBase");

		// I don't know why this needs to be aligned up to 8 on Win32. :|
		static_assert(alignUp(T::Type::getNumComponents() * sizeof(void*), size_t(8)) == storageSize, "Family type has unexpected storage size");

		struct StorageType : public FamilyBase
		{
			alignas(alignof(void*)) std::array<char, storageSize> data;

			bool operator<(const StorageType& other) const
			{
				if (T::Type::getNumComponents() == 0) {
					return entityId < other.entityId;
				} else {
					// Compare first component pointer
					using Type = void*;
					const auto& myC0 = reinterpret_cast<const Type*>(data.data())[0];
					const auto& otherC0 = reinterpret_cast<const Type*>(other.data.data())[0];
					return myC0 < otherC0;
				}
			}
		};

	public:
		explicit FamilyImpl(MaskStorage& storage)
			: Family(T::Type::inclusionMask(storage), T::ExclusionType::exclusionMask(storage), T::Type::optionalMask(storage))
		{
		}
			
	protected:
		void addEntity(Entity& entity) final
		{
			if (indexed) {
				index[entity.getEntityId()] = entities.size();
			}

			auto& e = entities.emplace_back();
			e.entityId = entity.getEntityId();
			T::Type::loadComponents(entity, &e.data[0]);

			dirty = true;
		}
		
		void refreshEntity(Entity& entity) final
		{
			if (auto* e = getEntityStorage(entity.getEntityId())) {
				T::Type::loadComponents(entity, &e->data[0]);
			}
		}
		
		void refreshEntityOptionals(Entity& entity) final
		{
			if (auto* e = getEntityStorage(entity.getEntityId())) {
				T::Type::loadOptionalComponents(entity, &e->data[0]);
			}
		}

		StorageType* getEntityStorage(EntityId id)
		{
			if (indexed) {
				if (auto iter = index.find(id); iter != index.end()) {
					return &entities[iter->second];
				}
			} else {
				for (auto& e: entities) {
					if (e.entityId == id) {
						return &e;
					}
				}
			}
			return nullptr;
		}

		void updateEntities() final
		{
			bool addedAny = false;
			if (dirty) {
				// Notify additions
				size_t prevSize = elemCount;
				size_t curSize = entities.size();
				updateElems();
				HalleyAssertDebug(curSize >= prevSize);
				dirty = false;

				if (curSize > prevSize) {
					notifyAdd(entities.data() + prevSize, curSize - prevSize);
					addedAny = true;
				}
			}

			if (!toReload.empty()) {
				// Notify reloads
				Vector<StorageType*> reloadedEntities;
				for (auto& entity : entities) {
					if (std::find(toReload.begin(), toReload.end(), entity.entityId) != toReload.end()) {
						reloadedEntities.push_back(&entity);
					}
				}
				notifyReload(reloadedEntities.data(), reloadedEntities.size());
				toReload.clear();
			}

			// Remove
			const bool removedAny = removeDeadEntities();

			// Remove will sort, but otherwise sort now
			if (addedAny && !removedAny) {
				sortElems();
			}
		}

		void clearEntities() final
		{
			notifyRemove(entities.data(), entities.size());
			entities.clear();
			updateElems();
			index.clear();
		}

		bool isDirty() const final
		{
			return dirty;
		}

		void rebuildIndex() final
		{
			index.clear();
			if (indexed) {
				const size_t n = entities.size();
				index.reserve(n);
				for (size_t i = 0; i < n; ++i) {
					index[entities[i].entityId] = i;
				}
			}
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

		bool removeDeadEntities()
		{
			const size_t removeCount = toRemove.size();
			if (removeCount == 0) {
				return false;
			}

			HalleyAssertDebug(removeCount <= entities.size());
			if (removeCount == entities.size()) {
				// If equal, they'll all be removed, so no need to re-arrange them
				toRemove.clear();
			} else {
				if (removeCount < 100) {
					moveDeadEntitiesToBackLinear();
				} else {
					moveDeadEntitiesToBackHash();
				}
			}

			// Notify removal
			size_t newSize = entities.size() - removeCount;
			HalleyAssertDebug(newSize < entities.size());
			notifyRemove(entities.data() + newSize, removeCount);

			// Remove them
			entities.resize(newSize);
			updateElems();
			sortElems();
			rebuildIndex();
			return true;
		}

		void moveDeadEntitiesToBackLinear()
		{
			// Performance-critical code
			// Benchmarks suggest that using a Vector is faster than std::set and std::unordered_set
			std::sort(toRemove.begin(), toRemove.end());

			// Move all entities to be removed to the back of the vector
			int n = int(entities.size());
			// Note: it's important to scan it forward. Scanning backwards would improve performance for short-lived entities,
			// but it causes an issue where an entity is removed and added to the same family in one frame.
			for (int i = 0; i < n; i++) {
				const EntityId id = entities[i].entityId;
				const auto iter = std::lower_bound(toRemove.begin(), toRemove.end(), id);
				if (iter != toRemove.end() && id == *iter) {
					toRemove.erase(iter);
					if (i != n - 1) [[likely]] {
						std::swap(entities[i], entities[n - 1]);
						--i;
					}
					--n;
					if (toRemove.empty()) [[unlikely]] {
						break;
					}
				}
			}
		}
		
		void moveDeadEntitiesToBackHash()
		{
			struct FastEntityHasher {
				constexpr uint64_t operator()(const EntityId& id) const noexcept
				{
					return Hash::hash(id.value);
				}
			};

			static thread_local HashSet<EntityId, FastEntityHasher> toRemoveIds;
			toRemoveIds.reserve(toRemove.size());
			for (auto& id: toRemove) {
				toRemoveIds.insert(id);
			}
			toRemove.clear();

			int n = int(entities.size());
			// Note: it's important to scan it forward. Scanning backwards would improve performance for short-lived entities,
			// but it causes an issue where an entity is removed and added to the same family in one frame.
			for (int i = 0; i < n; i++) {
				const EntityId id = entities[i].entityId;
				if (toRemoveIds.contains(id)) {
					if (i != n - 1) [[likely]] {
						std::swap(entities[i], entities[n - 1]);
						--i;
					}
					--n;
				}
			}
			toRemoveIds.clear();
		}

		void sortElems()
		{
			//std::sort(entities.begin(), entities.end());
		}
	};
}
