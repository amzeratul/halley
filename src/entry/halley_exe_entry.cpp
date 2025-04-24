#include <halley.hpp>
using namespace Halley;

IHalleyEntryPoint* getHalleyEntryStatic();

#if defined(WITH_SDL3)

#include <SDL3/SDL_main.h>

extern "C" int main(int argc, char* argv[])
{
    return HalleyMain::runMain(std::make_unique<EntryPointGameLoader>(*getHalleyEntryStatic()), HalleyMain::getArgs(argc, argv));
}

#elif defined(_WIN32) || defined(WITH_GDK)

int __stdcall WinMain(void*, void*, char*, int)
{
    return HalleyMain::runMain(std::make_unique<EntryPointGameLoader>(*getHalleyEntryStatic()), HalleyMain::getWin32Args());
}

#else

int main(int argc, char* argv[])
{
    return HalleyMain::runMain(std::make_unique<EntryPointGameLoader>(*getHalleyEntryStatic()), HalleyMain::getArgs(argc, argv));
}

#endif
