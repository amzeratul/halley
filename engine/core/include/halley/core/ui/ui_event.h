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
		ListAccept
	};

    class UIEvent {
    public:
		UIEvent();
		UIEvent(UIEventType type, String sourceId, String data = "");
		
    	UIEventType getType() const;
		String getSourceId() const;
		String getData() const;

    private:
		UIEventType type;
		String sourceId;
		String data;
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
