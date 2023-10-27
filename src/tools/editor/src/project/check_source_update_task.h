#pragma once

#include <halley.hpp>

namespace Halley {
	class ProjectWindow;

	class CheckSourceUpdateTask : public Task {
    public:
        CheckSourceUpdateTask(ProjectWindow& projectWindow, Path projectPath);

	protected:
        void run() override;

    private:
        ProjectWindow& projectWindow;
        Path projectPath;
        DirectoryMonitor monitorSource;
        DirectoryMonitor monitorCurrent;
        bool firstCheck = true;

        String lastSrcHash;
        String lastReadFile;

        bool needsUpdate();
    };

    
    class UpdateSourceTask : public Task {
    public:
        UpdateSourceTask(ProjectWindow& projectWindow);

	protected:
        void run() override;
        std::optional<String> getAction() override;
        void doAction(TaskSet& taskSet) override;

    private:
        ProjectWindow& projectWindow;
    };
    
}
