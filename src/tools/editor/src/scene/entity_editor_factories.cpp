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

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "textValue", context.getFactory().getStyle("input"), value, LocalisedString::fromUserString(defaultValue));
		field->bindData("textValue", value, [&, fieldName](String newVal)
		{
			componentData[fieldName] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});
		
		field->setMinSize(Vector2f(60, 25));

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

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "intValue", context.getFactory().getStyle("input"));
		field->setValidator(std::make_shared<UINumericValidator>(true, false));
		field->setMinSize(Vector2f(60, 25));
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

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "floatValue", context.getFactory().getStyle("input"));
		field->setValidator(std::make_shared<UINumericValidator>(true, true));
		field->setMinSize(Vector2f(60, 25));
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
		Maybe<Vector2f> value;
		if (componentData[fieldName].getType() != ConfigNodeType::Undefined) {
			value = componentData[fieldName].asVector2f();
		}
		Vector2f defVecValue;

		auto x = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "xValue", context.getFactory().getStyle("input"));
		x->setValidator(std::make_shared<UINumericValidator>(true, true));
		x->setMinSize(Vector2f(60, 25));
		x->bindData("xValue", value.value_or(defVecValue).x, [&, fieldName] (float newVal)
		{
			auto& node = componentData[fieldName];
			node = ConfigNode(Vector2f(newVal, node.asVector2f(Vector2f()).y));
			context.onEntityUpdated();
		});

		auto y = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "yValue", context.getFactory().getStyle("input"));
		y->setValidator(std::make_shared<UINumericValidator>(true, true));
		y->setMinSize(Vector2f(60, 25));
		y->bindData("yValue", value.value_or(defVecValue).y, [&, fieldName](float newVal)
		{
			auto& node = componentData[fieldName];
			node = ConfigNode(Vector2f(node.asVector2f(Vector2f()).x, newVal));
			context.onEntityUpdated();
		});

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
	factories.emplace_back(std::make_unique<ComponentEditorAngle1fFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorBoolFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorVector2fFieldFactory>());
	
	return factories;
}
