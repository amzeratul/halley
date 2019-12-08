#include "editor_ui_factory.h"
#include "halley/core/resources/resources.h"
#include <halley/file_formats/config_file.h>
using namespace Halley;

std::shared_ptr<UIStyleSheet> makeStyleSheet(Resources& resources)
{
	auto result = std::make_shared<UIStyleSheet>(resources);
	for (auto& style: resources.enumerate<ConfigFile>()) {
		if (style.startsWith("ui_style/")) {
			result->load(*resources.get<ConfigFile>(style));
		}
	}
	return result;
}


EditorUIFactory::EditorUIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n)
	: UIFactory(api, resources, i18n, makeStyleSheet(resources))
{
	UIInputButtons listButtons;
	setInputButtons("list", listButtons);
}
