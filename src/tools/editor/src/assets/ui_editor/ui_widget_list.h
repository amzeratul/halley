#pragma once

namespace Halley {
    class UIWidgetList : public UIWidget {
    public:
        UIWidgetList(String id, UIFactory& factory);

        void onMakeUI() override;

    private:
        UIFactory& factory;
    };
}
