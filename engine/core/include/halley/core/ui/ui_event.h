#pragma once
#include <functional>
#include "halley/text/halleystring.h"
#include <map>

namespace Halley {
	enum class UIEventType {
		ButtonClicked,
		CheckboxUpdated
	};

    class UIEvent {
    public:
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

		bool handle(const UIEvent& event) const;

	private:
		std::map<UIEventType, UIEventCallback> handles;
		std::map<std::pair<UIEventType, String>, UIEventCallback> specificHandles;
	};
}
