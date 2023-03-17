#pragma once
#include "project_dll.h"

namespace Halley {
	class ProjectWindow;

	class LoadDLLTask final : public Task {
	public:
		LoadDLLTask(IProjectWindow& projectWindow, ProjectDLL::Status status);

	protected:
		void run() override;
		std::optional<String> getAction() override;
		void doAction() override;

	private:
		IProjectWindow& projectWindow;
		ProjectDLL::Status status;
	};
}
