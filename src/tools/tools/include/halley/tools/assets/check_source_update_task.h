#pragma once

#include "import_assets_database.h"
#include "halley/concurrency/task.h"
#include "halley/file/directory_monitor.h"

namespace Halley {
	class Project;

	class CheckSourceUpdateTask : public Task {
    public:
        CheckSourceUpdateTask(Project& project, bool autoBuild, bool oneShot);

	protected:
        void run() override;

    private:
        Project& project;
        const bool autoBuild;
        const bool oneShot;

        DirectoryMonitor monitorSource;
        DirectoryMonitor monitorCurrent;
        bool firstCheck = true;

        String lastSrcHash;
        String lastReadFile;
        String lastSourceListsHash;

        bool needsUpdate();
		void generateSourceListing();
    };

    
    class UpdateSourceTask : public Task {
    public:
        UpdateSourceTask(Project& project, bool reportError);

	protected:
        void run() override;

        std::optional<String> getAction() override;
        void doAction(TaskSet& taskSet) override;

    private:
        Project& project;
        const bool reportError;
    };
    
}
