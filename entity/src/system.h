#pragma once

#include <vector>
#include <initializer_list>

#include "family_binding.h"
#include "family_mask.h"
#include "family_type.h"
#include "entity.h"

namespace Halley {
	class System
	{
	public:
		System(std::initializer_list<FamilyBindingBase*> uninitializedFamilies);
		virtual ~System() {}
		void step();

	protected:
		virtual void tick(Time time) = 0;

	private:
		friend class World;

		int nsTaken;
		std::vector<FamilyBindingBase*> families;
		World* world;
		String name;

		void onAddedToWorld(World& world);
	};

}
