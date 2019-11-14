#include "widgets/ui_debug_console.h"
#include "widgets/ui_label.h"
#include "widgets/ui_textinput.h"
#include "widgets/ui_scrollbar_pane.h"
#include "ui_factory.h"

Halley::UIDebugConsole::UIDebugConsole(const String& id, UIFactory& factory)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical))
{
	UIWidget::add(factory.makeUI("ui/halley/debug_console"));
}
