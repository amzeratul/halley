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

        virtual Vector<GamePlatform> getBuildPlatforms() { return {}; }
        virtual void launchGame(GamePlatform platform, OS& os, const IProject* project, const Vector<String>& params) {}
        virtual std::unique_ptr<Task> deployGame(GamePlatform platform, OS& os, const IProject& project) { return {}; }
        virtual std::unique_ptr<Task> buildGame(GamePlatform platform, OS& os, const IProject& project) { return {}; }
        virtual bool canDeployGame(GamePlatform platform) const { return false; }
        virtual std::optional<String> getDeviceNameConnectingFrom(const String& ip) const { return std::nullopt; }
    };
}
