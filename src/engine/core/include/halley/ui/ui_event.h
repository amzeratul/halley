#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/text/halleystring.h"
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"

namespace Halley {
	enum class KeyCode;
	enum class KeyMods : uint8_t;
	class UIWidget;

	enum class UIEventType : uint16_t {
		Undefined,

		// Please keep these in alphabetical order
		AssetsReloaded,
		ButtonClicked,
		ButtonDoubleClicked,
		ButtonRightClicked,
		CanvasDoubleClicked,
		CheckboxUpdated,
		Dragged,
		DropdownSelectionChanged,
		DropdownHoveredChanged,
		DropdownOpened,
		DropdownClosed,
		FocusGained,
		FocusLost,
		GroupFocusChangeRequested,
		ImageUpdated,
		KeyPressed,
		ListSelectionChanged,
		ListAccept,
		ListCancel,
		ListItemLeftClicked,
		ListItemMiddleClicked,
		ListItemRightClicked,
		ListBackgroundLeftClicked,
		ListBackgroundMiddleClicked,
		ListBackgroundRightClicked,
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
		MakeAreaVisibleContinuous,
		NavigateToAsset,
		NavigateToFile,
		PopupAccept,
		PopupHoveredChanged,
		PopupSelectionChanged,
		PopupCanceled,
		ReloadData,
		ScrollPositionChanged,
		SetSelected,
		SetEnabled,
		SetHovered,
		SpinControlValueChanged,
		TextChanged,
		TextSubmit,
		TabChanged,
		TabbedIn,
		TabbedOut,
		TreeCollapseHandle,
		TreeExpandHandle,
		TreeItemExpanded,
		TreeItemReparented,
		UnhandledMousePressLeft,
		UnhandledMousePressRight,
		UnhandledMousePressMiddle,
		UnhandledMouseReleaseLeft,
		UnhandledMouseReleaseRight,
		UnhandledMouseReleaseMiddle,
		WidgetHighlighted,
	};

	enum class UIEventDirection: uint8_t {
		Undefined,
		Up,  // i.e. towards the root
		Down // i.e. towards the leafs
	};

    class UIEvent {
    public:
		UIEvent();
		UIEvent(UIEventType type, String sourceId, String data = "");
		UIEvent(UIEventType type, String sourceId, bool data);
		UIEvent(UIEventType type, String sourceId, bool data1, bool data2);
		UIEvent(UIEventType type, String sourceId, int data);
		UIEvent(UIEventType type, String sourceId, int data1, int data2);
		UIEvent(UIEventType type, String sourceId, int data, KeyMods keyMods);
		UIEvent(UIEventType type, String sourceId, KeyCode keyCode, KeyMods keyMods);
		UIEvent(UIEventType type, String sourceId, float data);
		UIEvent(UIEventType type, String sourceId, String data, int intData);
		UIEvent(UIEventType type, String sourceId, String data, bool boolData);
		UIEvent(UIEventType type, String sourceId, String data, String data2, int intData);
		UIEvent(UIEventType type, String sourceId, Vector2f data);
		UIEvent(UIEventType type, String sourceId, Vector2f data, int intData, KeyMods keyMods);
		UIEvent(UIEventType type, String sourceId, Rect4f data);
		UIEvent(UIEventType type, String sourceId, ConfigNode data);
		
    	UIEventType getType() const;
		const String& getSourceId() const;
		[[deprecated]] String getData() const;
    	String getStringData() const;
    	String getStringData2() const;
		bool getBoolData() const;
		bool getBoolData2() const;
		int getIntData() const;
		int getIntData2() const;
    	KeyCode getKeyCode() const;
    	KeyMods getKeyMods() const;
		float getFloatData() const;
		Vector2f getVectorData() const;
		Rect4f getRectData() const;
		const ConfigNode& getConfigData() const;
		UIWidget& getCurWidget() const;
		UIEventDirection getDirection() const;

	    void setCurWidget(UIWidget* widget);
		void setDirection(UIEventDirection direction);

    private:
		UIEventType type = UIEventType::Undefined;
		UIEventDirection direction = UIEventDirection::Undefined;
		String sourceId;
		ConfigNode configData;
		UIWidget* curWidget = nullptr;
	};

	using UIEventCallback = std::function<void(const UIEvent&)>;
}
