#pragma once
#include <functional>
#include "halley/text/halleystring.h"
#include <map>
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"
#include "ui_parent.h"
#include "halley/core/input/input_keys.h"
#include "ui_event.h"

namespace Halley {
	class UIEventHandler {
	public:
		void setHandle(UIEventType type, UIEventCallback handler);
		void setHandle(UIEventType type, const String& id, UIEventCallback handler);
		void clearHandle(UIEventType type);
		void clearHandle(UIEventType type, const String& id);

		bool canHandle(const UIEvent& event) const;
		void queue(UIEvent event);
		void pump();
		void setWidget(UIWidget* uiWidget);

	private:
		UIWidget* widget = nullptr;
		std::map<UIEventType, UIEventCallback> handles;
		std::map<std::pair<UIEventType, String>, UIEventCallback> specificHandles;

		Vector<UIEvent> eventQueue;

		void handle(UIEvent& event);
	};
}
