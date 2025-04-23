#include <halley.hpp>
using namespace Halley;

IHalleyEntryPoint* getHalleyEntryStatic();


HALLEY_EXPORT IHalleyEntryPoint* getHalleyEntry()
{
    return getHalleyEntryStatic();
}

// Dummy implementation
void initSteamPlugin(Halley::IPluginRegistry& registry, int steamId, Halley::String initialLobby) {}

#if defined(WITH_SDL3)
void SDL_GDKSuspendComplete_Proxy() {}
#endif
