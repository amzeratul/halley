#include "editor_ui_factory.h"
using namespace Halley;

EditorUIFactory::EditorUIFactory(Resources& resources)
	: resources(resources)
	, styleSheet(resources.get<ConfigFile>("ui_style/editor")->getRoot(), resources)
{
}

std::shared_ptr<UILabel> EditorUIFactory::makeLabel(const String& label)
{
	return std::make_shared<UILabel>(styleSheet.getTextRenderer("label"), LocalisedString::fromHardcodedString(label));
}
