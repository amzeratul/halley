#pragma once

#include "halley/time/halleytime.h"
#include "halley/concurrency/future.h"

namespace Halley {
	class OS;
	class IProject;

	class IHalleyEditorPlugin {
    public:
        virtual ~IHalleyEditorPlugin() = default;

    	virtual void update(Time time) {}

        virtual std::optional<GamePlatform> getBuildPlatform() { return {}; }
        virtual void launchGame(OS& os, const IProject* project, const Vector<String>& params) {}
        virtual Future<int> buildGame(OS& os, IProject* project, ILoggerSink* logger) { return Future<int>::makeImmediate(1); }
    };
}
