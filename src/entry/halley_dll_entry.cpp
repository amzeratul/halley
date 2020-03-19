#include <halley.hpp>
using namespace Halley;

IHalleyEntryPoint* getHalleyEntryStatic();


HALLEY_EXPORT IHalleyEntryPoint* getHalleyEntry()
{
    return getHalleyEntryStatic();
}
