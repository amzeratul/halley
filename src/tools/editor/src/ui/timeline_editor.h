#pragma once

#include "prec.h"

namespace Halley
{
    class ITimelineEditorCallbacks {
    public:
        virtual ~ITimelineEditorCallbacks() = default;

        virtual void onStartTimelineRecording() = 0;
        virtual void onStopTimelineRecording() = 0;
        virtual void saveTimeline(const String& id, const Timeline& timeline) = 0;
    };

    class TimelineEditor : public UIWidget {
    public:
    	TimelineEditor(String id, UIFactory& factory);
        ~TimelineEditor() override;

        void onMakeUI() override;

    	void open(const String& id, std::unique_ptr<Timeline> timeline, ITimelineEditorCallbacks& callbacks);
        bool isRecording() const;

    	void addChange(const String& targetId, const String& fieldGroupId, const String& fieldId, const String& fieldSubKeyId, const ConfigNode& data);

    private:
        String timelineOwnerId;
        std::unique_ptr<Timeline> timeline;
		ITimelineEditorCallbacks* callbacks = nullptr;

        bool recording = false;
        int curFrame = 0;

        void populateTimeline();
        void saveTimeline();

        void toggleRecording();
    };
}
