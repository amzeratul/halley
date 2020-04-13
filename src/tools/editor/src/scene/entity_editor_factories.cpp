#include "entity_editor_factories.h"

#include "src/ui/select_asset_widget.h"
using namespace Halley;

class ComponentEditorTextFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::String";
	}

	std::shared_ptr<IUIElement> createField(ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto& componentData = pars.componentData;
		const auto& componentName = pars.componentName;
		const auto& fieldName = pars.fieldName;
		const auto& defaultValue = pars.defaultValue;

		String value = componentData[fieldName].asString("");

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "textValue", context.getFactory().getStyle("inputThin"), value, LocalisedString::fromUserString(defaultValue));
		field->bindData("textValue", value, [&, fieldName](String newVal)
		{
			componentData[fieldName] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		return field;
	}
};

class ComponentEditorIntFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "int";
	}

	std::shared_ptr<IUIElement> createField(ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto& componentData = pars.componentData;
		const auto& componentName = pars.componentName;
		const auto& fieldName = pars.fieldName;
		const auto& defaultValue = pars.defaultValue;

		int value = componentData[fieldName].asInt(defaultValue.isInteger() ? defaultValue.toInteger() : 0);

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "intValue", context.getFactory().getStyle("inputThin"));
		field->setValidator(std::make_shared<UINumericValidator>(true, false));
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

	std::shared_ptr<IUIElement> createField(ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto& componentData = pars.componentData;
		const auto& componentName = pars.componentName;
		const auto& fieldName = pars.fieldName;
		const auto& defaultValue = pars.defaultValue;

		const float value = componentData[fieldName].asFloat(defaultValue.isNumber() ? defaultValue.toFloat() : 0.0f);

		auto field = std::make_shared<UITextInput>(context.getFactory().getKeyboard(), "floatValue", context.getFactory().getStyle("inputThin"));
		field->setValidator(std::make_shared<UINumericValidator>(true, true));
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

	std::shared_ptr<IUIElement> createField(ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto& componentData = pars.componentData;
		const auto& componentName = pars.componentName;
		const auto& fieldName = pars.fieldName;
		const auto& defaultValue = pars.defaultValue;

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

	std::shared_ptr<IUIElement> createField(ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto& componentData = pars.componentData;
		const auto& componentName = pars.componentName;
		const auto& fieldName = pars.fieldName;
		const auto& defaultValue = pars.defaultValue;

		Vector2f value;
		if (componentData[fieldName].getType() != ConfigNodeType::Undefined) {
			value = componentData[fieldName].asVector2f();
		}

		const auto& keyboard = context.getFactory().getKeyboard();
		const auto& style = context.getFactory().getStyle("inputThin");

		auto dataOutput = std::make_shared<bool>(true);
		auto container = std::make_shared<UIWidget>(fieldName, Vector2f(), UISizer(UISizerType::Horizontal, 4.0f));

		container->add(std::make_shared<UITextInput>(keyboard, "xValue", style, "", LocalisedString(), std::make_shared<UINumericValidator>(true, true)), 1);
		container->bindData("xValue", value.x, [&, fieldName, dataOutput] (float newVal)
		{
			if (*dataOutput) {
				auto& node = componentData[fieldName];
				node = ConfigNode(Vector2f(newVal, node.asVector2f(Vector2f()).y));
				context.onEntityUpdated();
			}
		});

		container->add(std::make_shared<UITextInput>(keyboard, "yValue", style, "", LocalisedString(), std::make_shared<UINumericValidator>(true, true)), 1);
		container->bindData("yValue", value.y, [&, fieldName, dataOutput](float newVal)
		{
			if (*dataOutput) {
				auto& node = componentData[fieldName];
				node = ConfigNode(Vector2f(node.asVector2f(Vector2f()).x, newVal));
				context.onEntityUpdated();
			}
		});

		container->setHandle(UIEventType::ReloadData, componentName + ":" + fieldName, [=, &componentData] (const UIEvent& event)
		{
			Vector2f newVal;
			if (componentData[fieldName].getType() != ConfigNodeType::Undefined) {
				newVal = componentData[fieldName].asVector2f();
			}
			*dataOutput = false;
			event.getCurWidget().getWidgetAs<UITextInput>("xValue")->setText(toString(newVal.x));
			event.getCurWidget().getWidgetAs<UITextInput>("yValue")->setText(toString(newVal.y));
			*dataOutput = true;
		});

		return container;
	}
};

class ComponentEditorSpriteFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::Sprite";
	}

	bool isCompound() const override
	{
		return true;
	}

	std::shared_ptr<IUIElement> createField(ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto& componentData = pars.componentData;
		const auto& componentName = pars.componentName;
		const auto& fieldName = pars.fieldName;
		const auto& defaultValue = pars.defaultValue;

		auto& fieldData = componentData[fieldName];

		const auto& keyboard = context.getFactory().getKeyboard();
		const auto& inputStyle = context.getFactory().getStyle("inputThin");
		const auto& checkStyle = context.getFactory().getStyle("checkbox");

		auto pivotContainer = std::make_shared<UIWidget>(fieldName, Vector2f(), UISizer(UISizerType::Horizontal, 4.0f));
		pivotContainer->add(std::make_shared<UITextInput>(keyboard, "pivotX", inputStyle, "", LocalisedString(), std::make_shared<UINumericValidator>(true, true)), 1);
		pivotContainer->add(std::make_shared<UITextInput>(keyboard, "pivotY", inputStyle, "", LocalisedString(), std::make_shared<UINumericValidator>(true, true)), 1);

		auto container = std::make_shared<UIWidget>(fieldName, Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2));
		container->getSizer().setColumnProportions({{0, 1}});
		container->add(context.makeLabel("image"));
		container->add(std::make_shared<SelectAssetWidget>("image", context.getFactory(), AssetType::Sprite, context.getGameResources()));
		container->add(context.makeLabel("material"));
		container->add(std::make_shared<SelectAssetWidget>("material", context.getFactory(), AssetType::MaterialDefinition, context.getGameResources()));
		container->add(context.makeLabel("colour"));
		container->add(std::make_shared<UITextInput>(keyboard, "colour", inputStyle));
		container->add(context.makeLabel("pivot"));
		container->add(pivotContainer);
		container->add(context.makeLabel("flip"));
		container->add(std::make_shared<UICheckbox>("flip", checkStyle), 0, {}, UISizerAlignFlags::Left);
		container->add(context.makeLabel("visible"));
		container->add(std::make_shared<UICheckbox>("visible", checkStyle), 0, {}, UISizerAlignFlags::Left);

		container->bindData("image", fieldData["image"].asString(""), [&, fieldName](String newVal)
		{
			componentData[fieldName]["image"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});
		
		container->bindData("material", fieldData["material"].asString(""), [&, fieldName](String newVal)
		{
			componentData[fieldName]["material"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("colour", fieldData["colour"].asString("#FFFFFF"), [&, fieldName](String newVal)
		{
			componentData[fieldName]["colour"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("pivotX", fieldData["pivot"].asVector2f(Vector2f()).x, [&, fieldName] (float newVal)
		{
			auto& node = componentData[fieldName]["pivot"];
			node = ConfigNode(Vector2f(newVal, node.asVector2f(Vector2f()).y));
			context.onEntityUpdated();
		});
		
		container->bindData("pivotY", fieldData["pivot"].asVector2f(Vector2f()).y, [&, fieldName] (float newVal)
		{
			auto& node = componentData[fieldName]["pivot"];
			node = ConfigNode(Vector2f(node.asVector2f(Vector2f()).x, newVal));
			context.onEntityUpdated();
		});

		container->bindData("flip", fieldData["flip"].asBool(false), [&, fieldName](bool newVal)
		{
			componentData[fieldName]["flip"] = ConfigNode(newVal);
			context.onEntityUpdated();
		});

		container->bindData("visible", fieldData["visible"].asBool(true), [&, fieldName](bool newVal)
		{
			componentData[fieldName]["visible"] = ConfigNode(newVal);
			context.onEntityUpdated();
		});

		return container;
	}
};

class ComponentEditorAnimationPlayerFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::AnimationPlayer";
	}

	bool isCompound() const override
	{
		return true;
	}

	std::shared_ptr<IUIElement> createField(ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto& componentData = pars.componentData;
		const auto& componentName = pars.componentName;
		const auto& fieldName = pars.fieldName;
		const auto& defaultValue = pars.defaultValue;

		auto& fieldData = componentData[fieldName];

		auto& resources = context.getGameResources();
		const auto& keyboard = context.getFactory().getKeyboard();
		const auto& inputStyle = context.getFactory().getStyle("inputThin");
		const auto& checkStyle = context.getFactory().getStyle("checkbox");
		const auto& dropStyle = context.getFactory().getStyle("dropdown");
		const auto& scrollStyle = context.getFactory().getStyle("scrollbar");
		const auto& listStyle = context.getFactory().getStyle("list");

		auto container = std::make_shared<UIWidget>(fieldName, Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2));
		container->getSizer().setColumnProportions({{0, 1}});
		container->add(context.makeLabel("animation"));
		container->add(std::make_shared<SelectAssetWidget>("animation", context.getFactory(), AssetType::Animation, context.getGameResources()));
		container->add(context.makeLabel("sequence"));
		container->add(std::make_shared<UIDropdown>("sequence", dropStyle, scrollStyle, listStyle));
		container->add(context.makeLabel("direction"));
		container->add(std::make_shared<UIDropdown>("direction", dropStyle, scrollStyle, listStyle));
		container->add(context.makeLabel("playbackSpeed"));
		container->add(std::make_shared<UITextInput>(keyboard, "playbackSpeed", inputStyle, "", LocalisedString(), std::make_shared<UINumericValidator>(false, true)));
		container->add(context.makeLabel("applyPivot"));
		container->add(std::make_shared<UICheckbox>("applyPivot", checkStyle), 0, {}, UISizerAlignFlags::Left);

		auto updateAnimation = [container, fieldName, &resources, &componentData] (const String& animName)
		{
			std::vector<String> sequences;
			std::vector<String> directions;

			if (!animName.isEmpty()) {
				const auto anim = resources.get<Animation>(animName);
				directions = anim->getDirectionNames();
				sequences = anim->getSequenceNames();
			}

			auto sequence = container->getWidgetAs<UIDropdown>("sequence");
			auto direction = container->getWidgetAs<UIDropdown>("direction");
			sequence->setOptions(sequences);
			direction->setOptions(directions);
			sequence->setSelectedOption(componentData[fieldName]["sequence"].asString(""));
			direction->setSelectedOption(componentData[fieldName]["direction"].asString(""));
		};
		updateAnimation(fieldData["animation"].asString(""));

		container->bindData("animation", fieldData["animation"].asString(""), [&, fieldName, updateAnimation](String newVal)
		{
			updateAnimation(newVal);
			componentData[fieldName]["animation"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("sequence", fieldData["sequence"].asString(""), [&, fieldName](String newVal)
		{
			componentData[fieldName]["sequence"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("direction", fieldData["direction"].asString(""), [&, fieldName](String newVal)
		{
			componentData[fieldName]["direction"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("playbackSpeed", fieldData["playbackSpeed"].asFloat(1.0f), [&, fieldName](float newVal)
		{
			componentData[fieldName]["playbackSpeed"] = ConfigNode(newVal);
			context.onEntityUpdated();
		});

		container->bindData("applyPivot", fieldData["applyPivot"].asBool(true), [&, fieldName](bool newVal)
		{
			componentData[fieldName]["applyPivot"] = ConfigNode(newVal);
			context.onEntityUpdated();
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
	factories.emplace_back(std::make_unique<ComponentEditorSpriteFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorAnimationPlayerFieldFactory>());
	
	return factories;
}
