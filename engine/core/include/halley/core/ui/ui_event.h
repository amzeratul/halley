#pragma once
#include <functional>
#include "halley/text/halleystring.h"
#include <map>

namespace Halley {
	enum class UIEventType {
		Undefined,
		ButtonClicked,
		CheckboxUpdated,
		DropboxSelectionChanged,
		ListSelectionChanged,
		ListAccept,
		MouseWheel
	};

    class UIEvent {
    public:
		UIEvent();
		UIEvent(UIEventType type, String sourceId, String data = "");
		UIEvent(UIEventType type, String sourceId, int data);
		UIEvent(UIEventType type, String sourceId, float data);
		UIEvent(UIEventType type, String sourceId, String data, int intData);
		
    	UIEventType getType() const;
		String getSourceId() const;
		String getData() const;
		int getIntData() const;
		float getFloatData() const;

    private:
		UIEventType type;
		String sourceId;
		String data;
		int intData;
		float floatData;
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
