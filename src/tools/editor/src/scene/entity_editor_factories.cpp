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

		auto input = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "textValue", context.getFactory().getStyle("input"), value, LocalisedString::fromUserString(defaultValue));
		input->setMinSize(Vector2f(60, 25));

		return input;
	}
};

class ComponentEditorIntFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "int";
	}

	std::shared_ptr<IUIElement> createField(ComponentEditorContext& context, const String& fieldName, ConfigNode& componentData, const String& defaultValue) override
	{
		String value = componentData[fieldName].asString("");

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "intValue", context.getFactory().getStyle("input"), value, LocalisedString::fromHardcodedString(defaultValue));
		field->setValidator(std::make_shared<UINumericValidator>(true, false));
		field->setMinSize(Vector2f(60, 25));

		return field;
	}
};

class ComponentEditorFloatFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "float";
	}

	std::shared_ptr<IUIElement> createField(ComponentEditorContext& context, const String& fieldName, ConfigNode& componentData, const String& defaultValue) override
	{
		String value = componentData[fieldName].asString("");

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "floatValue", context.getFactory().getStyle("input"), value, LocalisedString::fromHardcodedString(defaultValue));
		field->setValidator(std::make_shared<UINumericValidator>(true, true));
		field->setMinSize(Vector2f(60, 25));

		return field;
	}
};

class ComponentEditorBoolFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "bool";
	}

	std::shared_ptr<IUIElement> createField(ComponentEditorContext& context, const String& fieldName, ConfigNode& componentData, const String& defaultValue) override
	{
		bool value = componentData[fieldName].asBool(defaultValue == "true");

		auto field = std::make_shared<UICheckbox>("boolValue", context.getFactory().getStyle("checkbox"), value);
		auto sizer = std::make_shared<UISizer>(UISizerType::Horizontal, 4.0f);
		sizer->add(field);
		
		return sizer;
	}
};

class ComponentEditorVector2fFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::Vector2f";
	}

	std::shared_ptr<IUIElement> createField(ComponentEditorContext& context, const String& fieldName, ConfigNode& componentData, const String& defaultValue) override
	{
		Maybe<Vector2f> value;
		if (componentData[fieldName].getType() != ConfigNodeType::Undefined) {
			value = componentData[fieldName].asVector2f();
		}

		auto x = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "xValue", context.getFactory().getStyle("input"), value ? toString(value->x) : "");
		x->setValidator(std::make_shared<UINumericValidator>(true, true));
		x->setMinSize(Vector2f(60, 25));

		auto y = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "yValue", context.getFactory().getStyle("input"), value ? toString(value->y) : "");
		y->setValidator(std::make_shared<UINumericValidator>(true, true));
		y->setMinSize(Vector2f(60, 25));

		auto sizer = std::make_shared<UISizer>(UISizerType::Horizontal, 4.0f);
		sizer->add(x, 1);
		sizer->add(y, 1);

		return sizer;
	}
};

std::vector<std::unique_ptr<IComponentEditorFieldFactory>> EntityEditorFactories::getDefaultFactories()
{
	std::vector<std::unique_ptr<IComponentEditorFieldFactory>> factories;

	factories.emplace_back(std::make_unique<ComponentEditorTextFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorIntFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorFloatFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorBoolFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorVector2fFieldFactory>());
	
	return factories;
}
