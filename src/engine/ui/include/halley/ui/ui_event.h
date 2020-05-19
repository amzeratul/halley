#pragma once
#include <functional>
#include "halley/text/halleystring.h"
#include <map>
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"
#include "ui_parent.h"
#include "halley/core/input/input_keys.h"

namespace Halley {
	enum class UIEventType {
		Undefined,

		// Please keep these in alphabetical order
		AssetsReloaded,
		ButtonClicked,
		CheckboxUpdated,
		Dragged,
		DropboxSelectionChanged,
		DropdownOpened,
		DropdownClosed,
		FocusGained,
		FocusLost,
		GroupFocusChangeRequested,
		KeyPressed,
		ListSelectionChanged,
		ListAccept,
		ListCancel,
		ListItemsSwapped,
		MouseWheel,
		MakeAreaVisible,
		MakeAreaVisibleCentered,
		ReloadData,
		SetSelected,
		SetEnabled,
		SetHovered,
		TextChanged,
		TextSubmit,
		TabChanged,
		TabbedIn,
		TabbedOut,
		TreeCollapse,
		TreeExpand,
		TreeItemReparented,
		WidgetHighlighted
	};

    class UIEvent {
    public:
		UIEvent();
		UIEvent(UIEventType type, String sourceId, String data = "");
		UIEvent(UIEventType type, String sourceId, bool data);
		UIEvent(UIEventType type, String sourceId, int data);
		UIEvent(UIEventType type, String sourceId, int data1, int data2);
		UIEvent(UIEventType type, String sourceId, KeyCode keyCode, KeyMods keyMods);
		UIEvent(UIEventType type, String sourceId, float data);
		UIEvent(UIEventType type, String sourceId, String data, int intData);
		UIEvent(UIEventType type, String sourceId, String data, String data2, int intData);
		UIEvent(UIEventType type, String sourceId, Vector2f data);
		UIEvent(UIEventType type, String sourceId, Rect4f data);
		
    	UIEventType getType() const;
		const String& getSourceId() const;
		[[deprecated]] String getData() const;
    	const String& getStringData() const;
    	const String& getStringData2() const;
		bool getBoolData() const;
		int getIntData() const;
		int getIntData2() const;
    	KeyCode getKeyCode() const;
    	KeyMods getKeyMods() const;
		float getFloatData() const;
		Vector2f getVectorData() const;
		Rect4f getRectData() const;
		UIWidget& getCurWidget() const;
	    
	    void setCurWidget(UIWidget* widget);

    private:
		UIEventType type;
		int intData = 0;
		int intData2 = 0;
		float floatData = 0.0f;
		String sourceId;
		String strData;
    	String strData2;
		Vector2f vectorData;
		Rect4f rectData;
		UIWidget* curWidget = nullptr;
		bool boolData = false;
	};

	using UIEventCallback = std::function<void(const UIEvent&)>;

	class UIEventHandler {
	public:
		void setHandle(UIEventType type, UIEventCallback handler);
		void setHandle(UIEventType type, const String& id, UIEventCallback handler);

		bool canHandle(const UIEvent& event) const;
		void queue(UIEvent event);
		void pump();
		void setWidget(UIWidget* uiWidget);

	private:
		UIWidget* widget = nullptr;
		std::map<UIEventType, UIEventCallback> handles;
		std::map<std::pair<UIEventType, String>, UIEventCallback> specificHandles;

		std::vector<UIEvent> eventQueue;

		void handle(UIEvent& event);
	};
}
