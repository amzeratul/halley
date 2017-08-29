#pragma once
#include <functional>
#include "halley/text/halleystring.h"
#include <map>
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"

namespace Halley {
	enum class UIEventType {
		Undefined,
		ButtonClicked,
		CheckboxUpdated,
		DropboxSelectionChanged,
		ListSelectionChanged,
		ListAccept,
		MouseWheel,
		MakeAreaVisible,
		MakeAreaVisibleCentered,
		Dragged,
		SetSelected,
		SetHovered
	};

    class UIEvent {
    public:
		UIEvent();
		UIEvent(UIEventType type, String sourceId, String data = "");
		UIEvent(UIEventType type, String sourceId, bool data);
		UIEvent(UIEventType type, String sourceId, int data);
		UIEvent(UIEventType type, String sourceId, float data);
		UIEvent(UIEventType type, String sourceId, String data, int intData);
		UIEvent(UIEventType type, String sourceId, Vector2f data);
		UIEvent(UIEventType type, String sourceId, Rect4f data);
		
    	UIEventType getType() const;
		String getSourceId() const;
		String getData() const;
		bool getBoolData() const;
		int getIntData() const;
		float getFloatData() const;
		Vector2f getVectorData() const;
		Rect4f getRectData() const;

    private:
		UIEventType type;
		String sourceId;
		String data;
		bool boolData;
		int intData;
		float floatData;
		Vector2f vectorData;
		Rect4f rectData;
    };

	using UIEventCallback = std::function<void(const UIEvent&)>;

	class UIEventHandler {
	public:
		void setHandle(UIEventType type, UIEventCallback handler);
		void setHandle(UIEventType type, String id, UIEventCallback handler);

		bool canHandle(const UIEvent& event) const;
		void queue(const UIEvent& event);
		void pump();

	private:
		std::map<UIEventType, UIEventCallback> handles;
		std::map<std::pair<UIEventType, String>, UIEventCallback> specificHandles;

		std::vector<UIEvent> eventQueue;

		void handle(const UIEvent& event) const;
	};
}
