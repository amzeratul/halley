#include "halley/tools/dll/load_dll_task.h"

using namespace Halley;

LoadDLLTask::LoadDLLTask(IProjectWindow& projectWindow, ProjectDLL::Status status)
	: Task("Loading Game DLL", false, true)
	, projectWindow(projectWindow)
	, status(status)
{
}

void LoadDLLTask::run()
{
	switch (status) {
	case ProjectDLL::Status::Loaded:
		setProgress(1.0f, "");
		return;

	case ProjectDLL::Status::Unloaded:
		logError("Game DLL was unloaded.");
		return;

	case ProjectDLL::Status::DLLNotFound:
		logError("Game DLL not found. Try building the game.");
		return;

	case ProjectDLL::Status::DLLVersionTooLow:
		logError("Game DLL is out of date. Try building the game.");
		return;

	case ProjectDLL::Status::DLLVersionTooHigh:
		logError("Editor is out of date. Try updating the editor.");
		return;

	case ProjectDLL::Status::DLLCrash:
		logError("Exception when trying to load game DLL.");
		return;

	case ProjectDLL::Status::InvalidDLL:
		logError("Game DLL does not seem to be a Halley DLL. Try building the game.");
		return;
	}
}

std::optional<String> LoadDLLTask::getAction()
{
	switch (status) {
	case ProjectDLL::Status::DLLNotFound:
	case ProjectDLL::Status::DLLVersionTooLow:
	case ProjectDLL::Status::InvalidDLL:
		return "Build Project";

	case ProjectDLL::Status::DLLVersionTooHigh:
		return "Update";

	default:
		return std::nullopt;
	}
}

void LoadDLLTask::doAction()
{
	switch (status) {
	case ProjectDLL::Status::DLLNotFound:
	case ProjectDLL::Status::DLLVersionTooLow:
	case ProjectDLL::Status::InvalidDLL:
		projectWindow.buildGame();
		return;

	case ProjectDLL::Status::DLLVersionTooHigh:
		projectWindow.updateEditor();
		return;
	}
}
