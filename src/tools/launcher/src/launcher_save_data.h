#pragma once

#include <halley.hpp>

namespace Halley {
    class LauncherSaveData {
    public:
    	LauncherSaveData(std::shared_ptr<ISaveData> saveData);

    	std::vector<Path> getProjectPaths() const;
    	void setProjectPaths(std::vector<Path> paths);

    	void save();
    	void load();

    private:
    	std::shared_ptr<ISaveData> saveData;
    	ConfigNode root;
    };
}
