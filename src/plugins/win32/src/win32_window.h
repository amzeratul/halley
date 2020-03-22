#pragma once

#include "halley/core/graphics/window.h"
#include "halley_windows.h"

namespace Halley {
    class Win32Window final : public Window {
    public:
		Win32Window(const WindowDefinition& definition);
	    void update(const WindowDefinition& definition) override;
	    void show() override;
	    void hide() override;
	    void setVsync(bool vsync) override;
	    void swap() override;
	    Rect4i getWindowRect() const override;
	    const WindowDefinition& getDefinition() const override;
		void destroy();

		void* getNativeHandle() override;
		String getNativeHandleType() override;
    	
	private:
		WindowDefinition definition;
		HWND hwnd;
    };
}
