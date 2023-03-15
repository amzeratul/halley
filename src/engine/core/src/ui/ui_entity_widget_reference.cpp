#include "halley/ui/ui_entity_widget_reference.h"
using namespace Halley;

UIEntityWidgetReference::UIEntityWidgetReference(String id, Vector2f offset, Vector2f alignment, std::shared_ptr<UIWidget> widget)
	: id(std::move(id))
	, offset(offset)
	, alignment(alignment)
	, widget(std::move(widget))
{
}
