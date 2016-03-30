#pragma once

#include "stage_id.h"

namespace Halley
{
	class HalleyAPI;
	class Painter;

	class Stage
	{
	public:
		virtual ~Stage() {}

		virtual void onFixedUpdate(Time) {}
		virtual void onVariableUpdate(Time) {}
		virtual void onRender(Painter&) const {}

		virtual void init() {}
		virtual void deInit() {}

	protected:
		explicit Stage(String name = "unnamed");

		HalleyAPI& getAPI() { return *api; }
		const HalleyAPI& getAPI() const { return *api; }

	private:
		friend class CoreRunner;

		void doInit(HalleyAPI* api);
		void doDeInit();

		String name;
		HalleyAPI* api;
	};

}
