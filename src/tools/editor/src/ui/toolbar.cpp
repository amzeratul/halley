#include "toolbar.h"
#include "halley/tools/project/project_properties.h"

using namespace Halley;

Toolbar::Toolbar(UIFactory& factory, const ProjectProperties& projectProperties)
	: UIWidget("toolbar", {}, UISizer())
{
	add(factory.makeUI("ui/halley/toolbar"), 1);

	getWidgetAs<UILabel>("gameName")->setText(LocalisedString::fromUserString(projectProperties.getName()));
}
