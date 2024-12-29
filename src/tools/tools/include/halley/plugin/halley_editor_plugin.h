#pragma once

#include "halley/time/halleytime.h"

namespace Halley {
    class IHalleyEditorPlugin {
    public:
        virtual ~IHalleyEditorPlugin() = default;

    	virtual void update(Time time) {}
    };
}
