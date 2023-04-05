#pragma once

#include <halley.hpp>

namespace Halley {
	class ProjectWindow;

	class CheckUpdateTask : public Task {
    public:
        CheckUpdateTask(ProjectWindow& projectWindow, Path projectPath);

	protected:
        void run() override;

    private:
        ProjectWindow& projectWindow;
        Path projectPath;
        DirectoryMonitor monitorAssets;
        bool firstCheck = true;

        bool needsUpdate();
        bool versionMatches();
    };

    class UpdateEditorTask : public Task {
    public:
        UpdateEditorTask(ProjectWindow& projectWindow);

	protected:
        void run() override;
        std::optional<String> getAction() override;
        void doAction(TaskSet& taskSet) override;

    private:
        ProjectWindow& projectWindow;
    };
}
