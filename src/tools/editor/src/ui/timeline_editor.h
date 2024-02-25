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

        void onMakeUI() override;

    	void open(const String& id, std::shared_ptr<Timeline> timeline, ITimelineEditorCallbacks& callbacks);
        bool isRecording() const;

    	void addChange(const String& targetId, const String& fieldGroupId, const String& fieldId, const String& fieldSubKeyId, const ConfigNode& data);

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
