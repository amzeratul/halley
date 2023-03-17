#pragma once

#include <halley.hpp>

namespace Halley {
    class CheckUpdateTask : public Task {
    public:
        CheckUpdateTask(Path projectPath);

	protected:
        void run() override;

    private:
        Path projectPath;
        DirectoryMonitor monitorAssets;
        bool firstCheck = true;

        bool needsUpdate();
        bool versionMatches();
    };

    class UpdateEditorTask : public Task {
    public:
        UpdateEditorTask();

	protected:
        void run() override;
    };
}
