#include "editor_ui_factory.h"
#include "halley/core/resources/resources.h"
#include <halley/file_formats/config_file.h>
using namespace Halley;

EditorUIFactory::EditorUIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n)
	: UIFactory(api, resources, i18n, std::make_shared<UIStyleSheet>(resources, *resources.get<ConfigFile>("ui_style/editor")))
{
	UIInputButtons listButtons;
	setInputButtons("list", listButtons);
}
