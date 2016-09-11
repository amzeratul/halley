#pragma once

#include "stage_id.h"
#include "halley/core/resources/resources.h"
#include "halley/core/api/halley_api.h"

namespace Halley
{
	class HalleyAPI;
	class RenderContext;
	class World;
	class InputAPI;
	class VideoAPI;
	class CoreAPI;

	class Stage
	{
	public:
		virtual ~Stage() {}

		virtual void onFixedUpdate(Time) {}
		virtual void onVariableUpdate(Time) {}
		virtual void onRender(RenderContext&) const {}

		virtual void init() {}
		virtual void deInit() {}

	protected:
		explicit Stage(String name = "unnamed");

		HalleyAPI& getAPI() { return *api; }
		const HalleyAPI& getAPI() const { return *api; }

		InputAPI& getInputAPI() const;
		VideoAPI& getVideoAPI() const;
		CoreAPI& getCoreAPI() const;
		Resources& getResources() const;

		template <typename T>
		std::shared_ptr<T> getResource(String name) const {
			return getResources().of<T>().get(name);
		}

	private:
		friend class Core;

		void doInit(HalleyAPI* api);
		void doDeInit();

		String name;
		HalleyAPI* api = nullptr;
	};

}
