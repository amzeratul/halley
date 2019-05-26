#pragma once

#include <halley/core/graphics/window.h>
#include <halley/maths/rect.h>

namespace Halley {
    class AndroidWindow : public Window {
    public:
        AndroidWindow(const WindowDefinition& definition);

        void update(const WindowDefinition& definition) override;
        void show() override;
        void hide() override;
        void setVsync(bool vsync) override;
        void swap() override;
        Rect4i getWindowRect() const override;
        const WindowDefinition& getDefinition() const override;

    private:
        WindowDefinition definition;
    };
}
