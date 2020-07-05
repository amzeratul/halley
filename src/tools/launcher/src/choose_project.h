#pragma once

#include <halley.hpp>

namespace Halley {
    class ChooseProject : public UIWidget {
    public:
    	ChooseProject(UIFactory& factory);

        void onMakeUI() override;
    	
    private:
    	UIFactory& factory;
    };
}