#pragma once

#include <array>
#include <vector>
#include "component.h"
#include "family_type.h"
#include "entity_id.h"

namespace Halley {
	class Entity
	{
		friend class World;
		static const int numFastComponents = 3;

	public:
		~Entity();

		template <typename T>
		Entity& addComponent(T* component)
		{
			static_assert(!std::is_same<T, Component>::value, "Cannot add base class Component to entity, make sure type isn't being erased");
			static_assert(std::is_base_of<Component, T>::value, "Components must extend the Component class");
			static_assert(!std::is_polymorphic<T>::value, "Components cannot be polymorphic (i.e. they can't have virtual methods)");
			static_assert(std::is_default_constructible<T>::value, "Components must have a default constructor");
			addComponent(component, T::componentIndex, getFastAccessSlot<T>(0));

			return *this;
		}

		template <typename T>
		Entity& removeComponent()
		{
			int id = T::componentIndex;
			for (size_t i = 0; i < components.size(); i++) {
				if (components[i].first == id) {
					deleteComponent(i->second, i->first, getFastAccessSlot<T>(0));
					components.erase(components.begin() + i);
					dirty = true;
					return *this;
				}
			}

			return *this;
		}

		template <typename T>
		T* getComponent()
		{
			const int fastAccessId = getFastAccessSlot<T>(0);
			if (fastAccessId >= 0) {
				return static_cast<T*>(fastComponents[fastAccessId]);
			}
			else {
				const int id = TypeUIDHelper<T>::get();
				for (size_t i = 0; i < components.size(); i++) {
					if (components[i].first == id) {
						return static_cast<T*>(components[i].second);
					}
				}
				return nullptr;
			}
		}

		template <typename T>
		bool hasComponent()
		{
			return ((ComponentMask(1) << TypeUIDHelper<T>::get()) & mask) != 0;
		}

		bool needsRefresh() const
		{
			return dirty;
		}

		bool isAlive() const {
			return alive;
		}

		FamilyMaskType getMask() const;
		EntityId getUID() const;

		void refresh();
		void destroy();

	private:
		std::vector<std::pair<int, Component*>> components;
		std::array<Component*, numFastComponents> fastComponents;
		FamilyMaskType mask;
		EntityId uid = 0;
		bool dirty;
		bool alive;

		Entity();

		template <typename T>
		int getFastAccessSlot(typename T::HasFastAccess*) {
			return T::fastAccessSlot < numFastComponents ? T::fastAccessSlot : -1;
		}

		template <typename>
		int getFastAccessSlot(...) {
			return -1;
		}

		void addComponent(Component* component, int id, int fastId);
		void deleteComponent(Component* component, int id, int fastId);
	};
}
