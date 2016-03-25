#pragma once
#include "family_type.h"
#include "entity_id.h"

namespace Halley {
	class Entity;

	class Family {
		friend class World;

	public:
		Family(FamilyMask::Type readMask, FamilyMask::Type writeMask);
		virtual ~Family() = default;

		virtual size_t count() const = 0;
		virtual void* getElement(size_t n) = 0;

	protected:
		virtual void addEntity(Entity& entity) = 0;
		virtual void removeEntity(Entity& entity) = 0;
		virtual void removeDeadEntities() = 0;
		virtual bool hasDeadEntities() const = 0;

		FamilyMask::Type getInclusionMask() const;
		FamilyMask::Type getMutableMask() const;
		FamilyMask::Type getConstMask() const;

	private:
		FamilyMask::Type inclusionMask;
		FamilyMask::Type writeMask;
		FamilyMask::Type readMask;
	};

	template <typename T>
	class FamilyImpl : public Family
	{
	public:
		FamilyImpl() : Family(T::Type::mask, T::Type::mask) {}

		std::vector<T>& getEntities()
		{
			return entities;
		}

		size_t count() const override
		{
			return entities.size();
		}

		void* getElement(size_t n) override
		{
			return &entities[n];
		}

	protected:
		void addEntity(Entity&) override
		{
			// TODO

			/*
			T tuple;
			TuplePopulator<T, std::tuple_size<T>::value - 1>::populateTuple(tuple, entity);
			entities.push_back(EntityEntry(entity.getUID(), tuple));
			*/
		}

		void removeEntity(Entity& entity) override
		{
			toRemove.push_back(entity.getUID());
		}

		bool hasDeadEntities() const override
		{
			return !toRemove.empty();
		}

		void removeDeadEntities() override
		{
			// Performance-critical code
			// Benchmarks suggest that using a std::vector is faster than std::set and std::unordered_set
			size_t size = entities.size();
			if (!toRemove.empty()) {
				std::sort(toRemove.begin(), toRemove.end());

				for (size_t i = 0; i < size; i++) {
					EntityId id = entities[i].entityId;
					auto iter = std::lower_bound(toRemove.begin(), toRemove.end(), id);
					if (iter != toRemove.end() && id == *iter) {
						toRemove.erase(iter);
						swapMemory(entities[i], entities[size - 1]);
						entities.pop_back();
						size--;
						i--;
						if (toRemove.empty()) break;
					}
				}
				toRemove.clear();
			}
		}

	private:
		std::vector<T> entities;
		std::vector<EntityId> toRemove;

		void swapMemory(T& a, T& b)
		{
			T c = a;
			memcpy(&a, &b, sizeof(T));
			memcpy(&b, &c, sizeof(T));
		}
	};
}
