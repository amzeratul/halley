#include "launcher_save_data.h"
using namespace Halley;

LauncherSaveData::LauncherSaveData(std::shared_ptr<ISaveData> saveData)
	: saveData(std::move(saveData))
{	
}

Vector<Path> LauncherSaveData::getProjectPaths() const
{
	return {};
}

void LauncherSaveData::setProjectPaths(Vector<Path> paths)
{}

void LauncherSaveData::save()
{}

void LauncherSaveData::load()
{}
