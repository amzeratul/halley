#pragma once

#include "stage_id.h"
#include "halley/core/resources/resources.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/game/frame_data.h"

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
		virtual ~Stage() = default;

		virtual void init();

		virtual void onStartFrame(Time dt);
		virtual void onFixedUpdate(Time dt);
		virtual void onVariableUpdate(Time dt);
		virtual void onRender(RenderContext& rc) const;

		virtual void onStartFrame(Time dt, IFrameData& frameData);
		virtual void onFixedUpdate(Time dt, IFrameData& frameData);
		virtual void onVariableUpdate(Time dt, IFrameData& frameData);
		virtual void onRender(RenderContext& rc, IFrameData& frameData) const;

		const HalleyAPI& getAPI() const;

		virtual bool onQuitRequested(); // Return true if OK to quit

		virtual std::unique_ptr<IFrameData> makeFrameData();
		virtual bool hasMultithreadedRendering() const;

	protected:
		explicit Stage(String name = "unnamed");

		InputAPI& getInputAPI() const;
		VideoAPI& getVideoAPI() const;
		AudioAPI& getAudioAPI() const;
		CoreAPI& getCoreAPI() const;
		SystemAPI& getSystemAPI() const;
		NetworkAPI& getNetworkAPI() const;
		MovieAPI& getMovieAPI() const;
		AnalyticsAPI& getAnalyticsAPI() const;
		Resources& getResources() const;

		Game& getGame() const;

		template <typename T>
		std::shared_ptr<const T> getResource(const String& name) const
		{
			return getResources().of<T>().get(name);
		}

	private:
		friend class Core;

		void setGame(Game& game);
		void doInit(const HalleyAPI* api, Resources& resources);
		void doDeInit();

		Game* game = nullptr;
		const HalleyAPI* api = nullptr;
		Resources* resources = nullptr;

		String name;
	};

}
