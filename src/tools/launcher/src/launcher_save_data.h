#pragma once

#include <halley.hpp>

namespace Halley {
    class LauncherSaveData {
    public:
    	LauncherSaveData(std::shared_ptr<ISaveData> saveData);

    	Vector<Path> getProjectPaths() const;
    	void setProjectPaths(Vector<Path> paths);

    	void save();
    	void load();

    private:
    	std::shared_ptr<ISaveData> saveData;
    	ConfigNode root;
    };
}
