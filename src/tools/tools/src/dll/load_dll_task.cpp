#include "halley/tools/dll/load_dll_task.h"

using namespace Halley;

LoadDLLTask::LoadDLLTask(ProjectDLL::Status status)
	: Task("Load Game DLL", false, true)
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
		logError("Editor is out of date. Try building the editor.");
		return;

	case ProjectDLL::Status::DLLCrash:
		logError("Exception when trying to load game DLL.");
		return;

	case ProjectDLL::Status::InvalidDLL:
		logError("Game DLL does not seem to be a Halley DLL. Try building the game.");
		return;
	}
}
