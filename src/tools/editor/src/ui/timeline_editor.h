#pragma once

#include "prec.h"

namespace Halley
{
    class ITimelineEditorCallbacks {
    public:
        virtual ~ITimelineEditorCallbacks() = default;

        virtual void saveTimeline(const String& id, const Timeline& timeline) = 0;
    };

    class TimelineEditor : public UIWidget {
    public:
    	TimelineEditor(String id, UIFactory& factory);

        void onMakeUI() override;

    	void open(const String& id, std::shared_ptr<Timeline> timeline, ITimelineEditorCallbacks& callbacks);
        bool isRecording() const;

    private:
        String targetId;
        std::shared_ptr<Timeline> timeline;
		ITimelineEditorCallbacks* callbacks = nullptr;

        bool recording = false;

        void loadTimeline();
        void saveTimeline();

        void toggleRecording();
    };
}
