#pragma once

#include "prec.h"

namespace Halley
{
    class TimelineEditor : public UIWidget {
    public:
        using SaveCallback = std::function<void(const String&, const Timeline&)>;

    	TimelineEditor(String id, UIFactory& factory);

        void onMakeUI() override;

    	void open(const String& id, std::shared_ptr<Timeline> timeline, SaveCallback callback);

    private:
        String targetId;
        std::shared_ptr<Timeline> timeline;
        SaveCallback callback;

        void loadTimeline();
        void saveTimeline();
    };
}
