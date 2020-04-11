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

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "textValue", context.getFactory().getStyle("inputThin"), value, LocalisedString::fromUserString(defaultValue));
		field->bindData("textValue", value, [&, fieldName](String newVal)
		{
			componentData[fieldName] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});
		
		field->setMinSize(Vector2f(60, 22));

		return field;
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
		int value = componentData[fieldName].asInt(defaultValue.isInteger() ? defaultValue.toInteger() : 0);

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "intValue", context.getFactory().getStyle("inputThin"));
		field->setValidator(std::make_shared<UINumericValidator>(true, false));
		field->setMinSize(Vector2f(60, 22));
		field->bindData("intValue", value, [&, fieldName](int newVal)
		{
			componentData[fieldName] = ConfigNode(newVal);
			context.onEntityUpdated();
		});

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
		const float value = componentData[fieldName].asFloat(defaultValue.isNumber() ? defaultValue.toFloat() : 0.0f);

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "floatValue", context.getFactory().getStyle("inputThin"));
		field->setValidator(std::make_shared<UINumericValidator>(true, true));
		field->setMinSize(Vector2f(60, 22));
		field->bindData("floatValue", value, [&, fieldName](float newVal)
		{
			componentData[fieldName] = ConfigNode(newVal);
			context.onEntityUpdated();
		});

		return field;
	}
};

class ComponentEditorAngle1fFieldFactory : public ComponentEditorFloatFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::Angle1f";
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
		field->bindData("boolValue", value, [&, fieldName](bool newVal)
		{
			componentData[fieldName] = ConfigNode(newVal);
			context.onEntityUpdated();
		});
		
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
		Vector2f value;
		if (componentData[fieldName].getType() != ConfigNodeType::Undefined) {
			value = componentData[fieldName].asVector2f();
		}

		const auto& keyboard = context.getFactory().getKeyboard();
		const auto& style = context.getFactory().getStyle("inputThin");

		auto dataOutput = std::make_shared<bool>(true);
		
		auto x = std::make_shared<UITextInput>(keyboard, "xValue", style);
		x->setValidator(std::make_shared<UINumericValidator>(true, true));
		x->setMinSize(Vector2f(60, 22));
		x->bindData("xValue", value.x, [&, fieldName, dataOutput] (float newVal)
		{
			if (*dataOutput) {
				auto& node = componentData[fieldName];
				node = ConfigNode(Vector2f(newVal, node.asVector2f(Vector2f()).y));
				context.onEntityUpdated();
			}
		});

		auto y = std::make_shared<UITextInput>(keyboard, "yValue", style);
		y->setValidator(std::make_shared<UINumericValidator>(true, true));
		y->setMinSize(Vector2f(60, 22));
		y->bindData("yValue", value.y, [&, fieldName, dataOutput](float newVal)
		{
			if (*dataOutput) {
				auto& node = componentData[fieldName];
				node = ConfigNode(Vector2f(node.asVector2f(Vector2f()).x, newVal));
				context.onEntityUpdated();
			}
		});

		auto container = std::make_shared<UIWidget>(fieldName, Vector2f(), UISizer(UISizerType::Horizontal, 4.0f));
		container->add(x, 1);
		container->add(y, 1);

		container->setHandle(UIEventType::ReloadData, fieldName, [=, &componentData] (const UIEvent& event)
		{
			Vector2f newVal;
			if (componentData[fieldName].getType() != ConfigNodeType::Undefined) {
				newVal = componentData[fieldName].asVector2f();
			}
			*dataOutput = false;
			x->setText(toString(newVal.x));
			y->setText(toString(newVal.y));
			*dataOutput = true;
		});

		return container;
	}
};

std::vector<std::unique_ptr<IComponentEditorFieldFactory>> EntityEditorFactories::getDefaultFactories()
{
	std::vector<std::unique_ptr<IComponentEditorFieldFactory>> factories;

	factories.emplace_back(std::make_unique<ComponentEditorTextFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorIntFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorFloatFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorAngle1fFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorBoolFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorVector2fFieldFactory>());
	
	return factories;
}
