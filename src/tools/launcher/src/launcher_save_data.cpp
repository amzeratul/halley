#include "launcher_save_data.h"
using namespace Halley;

LauncherSaveData::LauncherSaveData(std::shared_ptr<ISaveData> saveData)
	: saveData(std::move(saveData))
{	
}

std::vector<Path> LauncherSaveData::getProjectPaths() const
{
	return {};
}

void LauncherSaveData::setProjectPaths(std::vector<Path> paths)
{}

void LauncherSaveData::save()
{}

void LauncherSaveData::load()
{}
