#include "entity_editor_factories.h"
using namespace Halley;

class ComponentEditorTextFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::String";
	}

	std::shared_ptr<IUIElement> createField(ComponentEditorContext& context, const String& fieldName, ConfigNode& componentData, const String& defaultValue) override
	{
		String value = componentData[fieldName].asString("");

		auto input = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "", context.getFactory().getStyle("input"), value, LocalisedString::fromUserString(defaultValue));
		input->setMinSize(Vector2f(60, 25));

		return input;
	}
};

std::vector<std::unique_ptr<IComponentEditorFieldFactory>> EntityEditorFactories::getDefaultFactories()
{
	std::vector<std::unique_ptr<IComponentEditorFieldFactory>> factories;

	factories.emplace_back(std::make_unique<ComponentEditorTextFieldFactory>());
	
	return factories;
}
