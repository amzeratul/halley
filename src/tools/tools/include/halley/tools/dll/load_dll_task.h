#pragma once
#include "project_dll.h"

namespace Halley {
	class LoadDLLTask final : public Task {
	public:
		LoadDLLTask(ProjectDLL::Status status);

	protected:
		void run() override;

	private:
		ProjectDLL::Status status;
	};
}
