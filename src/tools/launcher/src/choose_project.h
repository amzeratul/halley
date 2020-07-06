#pragma once

#include <halley.hpp>

namespace Halley {
    class ChooseProject : public UIWidget {
    public:
    	ChooseProject(UIFactory& factory);

        void onMakeUI() override;
    	
    private:
    	UIFactory& factory;

    	std::vector<Path> paths;

    	void loadPaths();
    	void savePaths();
    	void addNewPath(Path path);
    	void addPath(Path path);
    	void refresh();
    };
}