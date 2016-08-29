#include "native_window.h"
#include "halley/os/os.h"

using namespace Halley;

#include <SDL.h>
#include <SDL_syswm.h>

void NativeWindow::setWindowIcon(SDL_Window* sdlWindow)
{
#ifdef _WIN32
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	if (SDL_GetWindowWMInfo(sdlWindow, &wminfo) == 1) {
		OS::get().onWindowCreated(wminfo.info.win.window);
	}
#endif
}
