#pragma once

#include "halley/core/graphics/text/text_renderer.h"
#include "halley/time/halleytime.h"
#include <cstdint>

#include "halley/core/input/input_exclusive.h"
#include "halley/time/stopwatch.h"

namespace Halley {
	class InputVirtual;
	class HalleyAPI;
    class Resources;
    class RenderContext;
    class World;

	struct StatsViewControls {
		int xAxis;
		int yAxis;
		int accept;
		int cancel;
		int nextTab;
		int prevTab;
	};

	class ScreenOverlay {
	public:
		ScreenOverlay();
		virtual ~ScreenOverlay() = default;
		
        virtual void draw(RenderContext& context);
		virtual void update(Time t);

	protected:
        virtual void paint(Painter& painter) = 0;
		Vector2f getScreenSize() const;

	private:
		Vector2f screenSize;
	};

	class StatsView : public ScreenOverlay {
    public:
        StatsView(Resources& resources, const HalleyAPI& api);
        virtual ~StatsView() = default;

		void update(Time t) override;
		void draw(RenderContext& context) override;

		void setActive(bool active);
		bool isActive() const;

		void setWorld(const World* world);
		void setInput(std::shared_ptr<InputVirtual> input, const StatsViewControls& controls);

    protected:
		enum Buttons {
			Accept,
			Cancel,
			PrevTab,
			NextTab
		};

        Resources& resources;
        const HalleyAPI& api;
        const World* world = nullptr;
		bool active = true;

		std::shared_ptr<InputExclusive> input;

        String formatTime(int64_t ns) const;
    };
}
