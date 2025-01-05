#pragma once

#include "halley/time/halleytime.h"
#include "halley/concurrency/future.h"
#include "halley/concurrency/task.h"

namespace Halley {
	class OS;
	class IProject;

	class IHalleyEditorPlugin {
    public:
        virtual ~IHalleyEditorPlugin() = default;

    	virtual void update(Time time) {}

        virtual std::optional<GamePlatform> getBuildPlatform() { return {}; }
        virtual void launchGame(OS& os, const IProject* project, const Vector<String>& params) {}
        virtual std::unique_ptr<Task> deployGame(OS& os, const IProject* project) { return {}; }
        virtual Future<int> buildGame(OS& os, IProject* project, ILoggerSink* logger) { return Future<int>::makeImmediate(1); }
        virtual bool canDeployGame() const { return false; }
    };
}
