#include "editor_ui_factory.h"
using namespace Halley;

EditorUIFactory::EditorUIFactory(Resources& resources, I18N& i18n)
	: UIFactory(resources, i18n, std::make_shared<UIStyleSheet>(resources.get<ConfigFile>("ui_style/editor")->getRoot(), resources))
{
}
