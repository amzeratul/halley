#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/text/halleystring.h"
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"

namespace Halley {
	enum class KeyCode;
	enum class KeyMods;
	class UIWidget;

	enum class UIEventType {
		Undefined,

		// Please keep these in alphabetical order
		AssetsReloaded,
		ButtonClicked,
		ButtonDoubleClicked,
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
		ListItemMiddleClicked,
		ListItemRightClicked,
		ListItemsSwapped,
		ListHoveredChanged,
		MouseWheel,
		MousePressLeft,
		MousePressRight,
		MousePressMiddle,
		MouseReleaseLeft,
		MouseReleaseRight,
		MouseReleaseMiddle,
		MakeAreaVisible,
		MakeAreaVisibleCentered,
		NavigateToAsset,
		NavigateToFile,
		PopupAccept,
		PopupHoveredChanged,
		PopupSelectionChanged,
		PopupCanceled,
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
		UnhandledMousePressLeft,
		UnhandledMousePressRight,
		UnhandledMousePressMiddle,
		UnhandledMouseReleaseLeft,
		UnhandledMouseReleaseRight,
		UnhandledMouseReleaseMiddle,
		WidgetHighlighted,
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
		UIEvent(UIEventType type, String sourceId, ConfigNode data);
		
    	UIEventType getType() const;
		const String& getSourceId() const;
		[[deprecated]] String getData() const;
    	String getStringData() const;
    	String getStringData2() const;
		bool getBoolData() const;
		int getIntData() const;
		int getIntData2() const;
    	KeyCode getKeyCode() const;
    	KeyMods getKeyMods() const;
		float getFloatData() const;
		Vector2f getVectorData() const;
		Rect4f getRectData() const;
		const ConfigNode& getConfigData() const;
		UIWidget& getCurWidget() const;
	    
	    void setCurWidget(UIWidget* widget);

    private:
		UIEventType type;
		String sourceId;
		ConfigNode configData;
		UIWidget* curWidget = nullptr;
	};

	using UIEventCallback = std::function<void(const UIEvent&)>;
}
