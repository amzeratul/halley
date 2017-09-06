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
	class Game;

	class Stage
	{
	public:
		virtual ~Stage() {}

		virtual void onFixedUpdate(Time) {}
		virtual void onVariableUpdate(Time) {}
		virtual void onRender(RenderContext&) const {}

		virtual void init() {}
		virtual void deInit() {}

		HalleyAPI& getAPI() { return *api; }
		const HalleyAPI& getAPI() const { return *api; }

	protected:
		explicit Stage(String name = "unnamed");

		InputAPI& getInputAPI() const;
		VideoAPI& getVideoAPI() const;
		AudioAPI& getAudioAPI() const;
		CoreAPI& getCoreAPI() const;
		SystemAPI& getSystemAPI() const;
		NetworkAPI& getNetworkAPI() const;
		Resources& getResources() const;

		Game& getGame() const;

		template <typename T>
		std::shared_ptr<const T> getResource(const String& name) const {
			return getResources().of<T>().get(name);
		}

	private:
		friend class Core;

		void setGame(Game& game);
		void doInit(HalleyAPI* api);
		void doDeInit();

		Game* game = nullptr;
		String name;
		HalleyAPI* api = nullptr;
	};

}
