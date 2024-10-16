#pragma once

#include <typeinfo>
#include "halley/text/halleystring.h"

namespace Halley
{
	class World;

	class Service
	{
	public:
		Service() = default;
		Service(const Service& other) = delete;
		Service(Service&& other) noexcept = default;
		virtual ~Service() = default;

		Service& operator=(const Service& other) = delete;
		Service& operator=(Service&& other) noexcept = default;

		String getName() const { return typeid(*this).name(); }

		void onAddedToWorld(World& world)
		{
			this->world = &world;
		}

	protected:
		World& getWorld() const
		{
			assert(world != nullptr);
			return *world;
		}

	private:
		World* world = nullptr;
	};
}