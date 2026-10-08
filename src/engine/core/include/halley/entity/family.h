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
					auto* e = getElement<T>(*idx);
					if (e->entityId == id) {
						return e;
					}
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

		bool isDirty() const
		{
			return dirty;
		}

		bool needsUpdate() const
		{
			return dirty || !toRemove.empty() || !toReload.empty();
		}

	protected:
		virtual void addEntity(Entity& entity) = 0;
		virtual void refreshEntity(Entity& entity) = 0;
		virtual void refreshEntityOptionals(Entity& entity) = 0;
		void removeEntity(EntityId::Index entityId);
		void reloadEntity(EntityId::Index entityId);
		virtual void updateEntities() = 0;
		virtual void clearEntities() = 0;
		bool hasRemoveCallbacks() const { return !removeEntityCallbacks.empty(); }

		OptionalLite<size_t> findElementInIndex(EntityId id) const;

		struct FastEntityHasher {
			constexpr uint64_t operator()(const EntityId& id) const noexcept
			{
				return Hash::hash(id.value);
			}
		};
		static Vector<uint8_t>& getScratchBitSet();

		void* elems = nullptr;
		size_t elemCount = 0;
		uint32_t elemSize = 0;
		bool indexed = false;
		bool dirty = false;
		
	private:
		FamilyMaskType inclusionMask;
		std::optional<FamilyMaskType> exclusionMask;
		FamilyMaskType optionalMask;

	protected:
		Vector<EntityId::Index> toRemove;
		Vector<EntityId::Index> toReload;

		Vector<FamilyBindingBase*> addEntityCallbacks;
		Vector<FamilyBindingBase*> removeEntityCallbacks;
		Vector<FamilyBindingBase*> modifiedEntityCallbacks;

		HashMap<EntityId::Index, uint32_t> index;
	};

	class FamilyBase {
	public:
		EntityId entityId;
	};

	template <typename T>
	class FamilyBaseOf : public FamilyBase {
	public:
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
				index[entity.getEntityId().getIndex()] = static_cast<uint32_t>(entities.size());
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
				if (auto iter = index.find(id.getIndex()); iter != index.end()) {
					auto* e = &entities[iter->second];
					if (e->entityId == id) {
						return e;
					}
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
				const size_t prevSize = elemCount;
				const size_t curSize = entities.size();
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
				// This is pretty slow, should probably use the bitmap algorithm that remove uses, but then again this is a dev-only feature
				Vector<StorageType*> reloadedEntities;
				for (auto& entity : entities) {
					if (std::find(toReload.begin(), toReload.end(), entity.entityId.getIndex()) != toReload.end()) {
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

		void rebuildIndex() final
		{
			index.clear();
			if (indexed) {
				const size_t n = entities.size();
				index.reserve(n);
				for (size_t i = 0; i < n; ++i) {
					index[entities[i].entityId.getIndex()] = static_cast<uint32_t>(i);
				}
			}
		}


	private:
		Vector<StorageType, std::allocator<StorageType>, 0, false> entities;

		static_assert(std::is_trivially_copyable_v<StorageType>);

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

			const bool hasCallbacks = hasRemoveCallbacks();

			HalleyAssertDebug(removeCount <= entities.size());
			if (removeCount == entities.size()) {
				// If equal, they'll all be removed, so no need to re-arrange them
				toRemove.clear();
			} else {
				// Performance-critical code
				moveDeadEntitiesToBackBitSet(hasCallbacks);
			}
			size_t newSize = entities.size() - removeCount;

			// Notify removal
			if (hasCallbacks) {
				notifyRemove(entities.data() + newSize, removeCount);
			}

			// Remove them
			entities.resize(newSize);
			updateElems();
			sortElems();
			rebuildIndex();
			return true;
		}

		void moveDeadEntitiesToBackBitSet(bool preserveRemoved)
		{
			// Should already be all zeroed out from last usage
			auto& scratch = getScratchBitSet();

			// Find range
			// We'll assume min is 0 as it simplifies the algorithm
			uint32_t max = std::numeric_limits<uint32_t>::min();
			for (auto& id: toRemove) {
				max = std::max(max, id);
			}

			// Resize if it needs to be bigger
			scratch.resize(std::max(scratch.size(), static_cast<size_t>(max / 8 + 1)), 0);
			auto bitmap = scratch.span();
			

			const auto contains = [&] (EntityId id) -> bool
			{
				const auto idx = id.getIndex();
				return idx <= max && (bitmap[idx / 8] & (1 << (idx % 8)));
			};
			
			const auto setBit = [&] (EntityId::Index idx)
			{
				bitmap[idx / 8] |= 1 << (idx % 8);
			};
			
			const auto clearBit = [&] (EntityId id)
			{
				const auto idx = id.getIndex();
				bitmap[idx / 8] &= ~static_cast<uint8_t>(1 << (idx % 8));
			};

			// Set bits
			for (auto& id: toRemove) {
				setBit(id);
			}
			const int nToRemove = static_cast<int>(toRemove.size());
			toRemove.clear();

			int n = static_cast<int>(entities.size());
			int nRemoved = 0;

			if (preserveRemoved) {
				// Will notify, so keep those at the end of vector
				for (int i = 0; i < n; i++) {
					if (contains(entities[i].entityId)) {
						clearBit(entities[i].entityId);
						if (i != n - 1) [[likely]] {
							std::swap(entities[i], entities[n - 1]);
							prefetchObjectL2(entities[n - 2]);
							--i;
						}
						--n;
						++nRemoved;
						if (nRemoved == nToRemove) [[unlikely]] {
							break;
						}
					}
				}
			} else {
				// Won't notify, just erase them
				for (int i = 0; i < n; i++) {
					if (contains(entities[i].entityId)) {
						clearBit(entities[i].entityId);
						if (i != n - 1) [[likely]] {
							entities[i] = std::move(entities[n - 1]);
							prefetchObjectL2(entities[n - 2]);
							--i;
						}
						--n;
						++nRemoved;
						if (nRemoved == nToRemove) [[unlikely]] {
							break;
						}
					}
				}
			}

			// Since we rely on all bits being set back to zero, this must happen
			if (nRemoved != nToRemove) {
				for (auto& v: bitmap) {
					v = 0;
				}
			}
			HalleyAssertDev(nRemoved == nToRemove);
		}

		void sortElems()
		{
			//std::sort(entities.begin(), entities.end());
		}
	};
}
