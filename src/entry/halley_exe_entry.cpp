#include <halley.hpp>
using namespace Halley;

IHalleyEntryPoint* getHalleyEntryStatic();


#if defined(_WIN32) || defined(WINDOWS_STORE)

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
