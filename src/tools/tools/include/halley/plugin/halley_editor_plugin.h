#pragma once

#include "halley/time/halleytime.h"
#include "halley/concurrency/future.h"

namespace Halley {
	class IProject;

	class IHalleyEditorPlugin {
    public:
        virtual ~IHalleyEditorPlugin() = default;

    	virtual void update(Time time) {}

        virtual std::optional<GamePlatform> getBuildPlatform() { return {}; }
        virtual void launchGame(const IProject* project, const Vector<String>& params) {}
        virtual Future<bool> buildGame(IProject* project, ILoggerSink* logger) { return Future<bool>::makeImmediate(false); }
    };
}
