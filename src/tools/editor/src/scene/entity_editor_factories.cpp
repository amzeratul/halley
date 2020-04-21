#include "entity_editor_factories.h"
#include "halley/core/graphics/sprite/animation.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_validator.h"
#include "halley/ui/widgets/ui_checkbox.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "src/ui/select_asset_widget.h"
#include "halley/core/scene_editor/component_field_parameters.h"
#include "halley/core/scene_editor/component_editor_context.h"

using namespace Halley;

class ComponentEditorTextFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::String";
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode("");
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;

		String value = data.getFieldData().asString("");

		auto field = std::make_shared<UITextInput>(context.getUIFactory().getKeyboard(), "textValue", context.getUIFactory().getStyle("inputThin"), value, LocalisedString::fromUserString(defaultValue));
		field->bindData("textValue", value, [&context, data](String newVal)
		{
			data.getFieldData() = ConfigNode(std::move(newVal));
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

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(0);
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;

		int value = data.getFieldData().asInt(defaultValue.isInteger() ? defaultValue.toInteger() : 0);

		auto field = std::make_shared<UITextInput>(context.getUIFactory().getKeyboard(), "intValue", context.getUIFactory().getStyle("inputThin"));
		field->setValidator(std::make_shared<UINumericValidator>(true, false));
		field->bindData("intValue", value, [&context, data](int newVal)
		{
			data.getFieldData() = ConfigNode(newVal);
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

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(0.0f);
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;

		const float value = data.getFieldData().asFloat(defaultValue.isNumber() ? defaultValue.toFloat() : 0.0f);

		auto field = std::make_shared<UITextInput>(context.getUIFactory().getKeyboard(), "floatValue", context.getUIFactory().getStyle("inputThin"));
		field->setValidator(std::make_shared<UINumericValidator>(true, true));
		field->bindData("floatValue", value, [&context, data](float newVal)
		{
			data.getFieldData() = ConfigNode(newVal);
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

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(false);
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;

		bool value = data.getFieldData().asBool(defaultValue == "true");

		auto field = std::make_shared<UICheckbox>("boolValue", context.getUIFactory().getStyle("checkbox"), value);
		field->bindData("boolValue", value, [&context, data](bool newVal)
		{
			data.getFieldData() = ConfigNode(newVal);
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

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(Vector2f());
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;

		Vector2f value;
		if (data.getFieldData().getType() != ConfigNodeType::Undefined) {
			value = data.getFieldData().asVector2f();
		}

		const auto& keyboard = context.getUIFactory().getKeyboard();
		const auto& style = context.getUIFactory().getStyle("inputThin");

		auto dataOutput = std::make_shared<bool>(true);
		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Horizontal, 4.0f));

		container->add(std::make_shared<UITextInput>(keyboard, "xValue", style, "", LocalisedString(), std::make_shared<UINumericValidator>(true, true)), 1);
		container->bindData("xValue", value.x, [&context, data, dataOutput] (float newVal)
		{
			if (*dataOutput) {
				auto& node = data.getFieldData();
				node = ConfigNode(Vector2f(newVal, node.asVector2f(Vector2f()).y));
				context.onEntityUpdated();
			}
		});

		container->add(std::make_shared<UITextInput>(keyboard, "yValue", style, "", LocalisedString(), std::make_shared<UINumericValidator>(true, true)), 1);
		container->bindData("yValue", value.y, [&context, data, dataOutput](float newVal)
		{
			if (*dataOutput) {
				auto& node = data.getFieldData();
				node = ConfigNode(Vector2f(node.asVector2f(Vector2f()).x, newVal));
				context.onEntityUpdated();
			}
		});

		container->setHandle(UIEventType::ReloadData, pars.componentName + ":" + data.getName(), [=] (const UIEvent& event)
		{
			Vector2f newVal;
			if (data.getFieldData().getType() != ConfigNodeType::Undefined) {
				newVal = data.getFieldData().asVector2f();
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

	bool isNested() const override
	{
		return true;
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(ConfigNode::MapType());
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;

		auto& fieldData = data.getFieldData();
		fieldData.ensureType(ConfigNodeType::Map);

		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2));
		container->getSizer().setColumnProportions({{0, 1}});
		container->add(context.makeLabel("image"));
		container->add(std::make_shared<SelectAssetWidget>("image", context.getUIFactory(), AssetType::Sprite, context.getGameResources()));
		container->add(context.makeLabel("material"));
		container->add(std::make_shared<SelectAssetWidget>("material", context.getUIFactory(), AssetType::MaterialDefinition, context.getGameResources()));
		container->add(context.makeLabel("colour"));
		container->add(context.makeField("Halley::Colour4f", pars.withSubKey("colour"), false));
		container->add(context.makeLabel("pivot"));
		container->add(context.makeField("std::optional<Halley::Vector2f>", pars.withSubKey("pivot"), false));
		container->add(context.makeLabel("flip"));
		container->add(context.makeField("bool", pars.withSubKey("flip"), false));
		container->add(context.makeLabel("visible"));
		container->add(context.makeField("bool", pars.withSubKey("visible", "true"), false));

		container->bindData("image", fieldData["image"].asString(""), [&context, data](String newVal)
		{
			data.getFieldData()["image"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("material", fieldData["material"].asString(""), [&context, data](String newVal)
		{
			data.getFieldData()["material"] = ConfigNode(std::move(newVal));
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

	bool isNested() const override
	{
		return true;
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(ConfigNode::MapType());
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;

		auto& fieldData = data.getFieldData();
		fieldData.ensureType(ConfigNodeType::Map);

		auto& resources = context.getGameResources();
		const auto& dropStyle = context.getUIFactory().getStyle("dropdown");
		const auto& scrollStyle = context.getUIFactory().getStyle("scrollbar");
		const auto& listStyle = context.getUIFactory().getStyle("list");

		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2));
		container->getSizer().setColumnProportions({{0, 1}});
		container->add(context.makeLabel("animation"));
		container->add(std::make_shared<SelectAssetWidget>("animation", context.getUIFactory(), AssetType::Animation, context.getGameResources()));
		container->add(context.makeLabel("sequence"));
		container->add(std::make_shared<UIDropdown>("sequence", dropStyle, scrollStyle, listStyle));
		container->add(context.makeLabel("direction"));
		container->add(std::make_shared<UIDropdown>("direction", dropStyle, scrollStyle, listStyle));
		container->add(context.makeLabel("playbackSpeed"));
		container->add(context.makeField("float", pars.withSubKey("playbackSpeed", "1"), false));
		container->add(context.makeLabel("applyPivot"));
		container->add(context.makeField("bool", pars.withSubKey("applyPivot", "true"), false));

		auto updateAnimation = [container, data, &resources] (const String& animName)
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
			sequence->setSelectedOption(data.getFieldData()["sequence"].asString(""));
			direction->setSelectedOption(data.getFieldData()["direction"].asString(""));
		};
		updateAnimation(fieldData["animation"].asString(""));

		container->bindData("animation", fieldData["animation"].asString(""), [&context, data, updateAnimation](String newVal)
		{
			updateAnimation(newVal);
			data.getFieldData()["animation"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("sequence", fieldData["sequence"].asString(""), [&context, data](String newVal)
		{
			data.getFieldData()["sequence"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("direction", fieldData["direction"].asString(""), [&context, data](String newVal)
		{
			data.getFieldData()["direction"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		return container;
	}
};

class ComponentEditorPolygonFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::Polygon";
	}

	virtual bool isOpenPolygon()
	{
		return false;
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(ConfigNode::SequenceType());
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.defaultValue;
		const auto componentNames = pars.otherComponentNames;
		const auto componentName = pars.componentName;

		auto style = context.getUIFactory().getStyle("buttonThin");

		auto field = std::make_shared<UIButton>("editPolygon", style, LocalisedString::fromHardcodedString("Edit..."));
		field->setMinSize(Vector2f(30, 22));

		field->setHandle(UIEventType::ButtonClicked, "editPolygon", [=, &context] (const UIEvent& event)
		{
			ConfigNode options = ConfigNode(ConfigNode::MapType());
			options["isOpenPolygon"] = isOpenPolygon();

			ConfigNode::SequenceType compNames;
			for (const auto& name : componentNames) {
				compNames.emplace_back(ConfigNode(name));
			}
			options["componentNames"] = std::move(compNames);

			context.setTool(SceneEditorTool::Polygon, componentName, data.getName(), std::move(options));
		});

		return field;
	}
};

class ComponentEditorVertexListFieldFactory : public ComponentEditorPolygonFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::VertexList";
	}

	bool isOpenPolygon() override
	{
		return true;
	}
};

class ComponentEditorStdVectorFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "std::vector<>";
	}

	bool isNested() const override
	{
		return false;
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(ConfigNode::SequenceType());
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		const auto fieldType = pars.typeParameters.at(0);
		const auto data = pars.data;

		data.getFieldData().ensureType(ConfigNodeType::Sequence);
		
		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Vertical));

		auto buildList = [=, &context] () {
			container->clear();
			
			const size_t nElements = data.getFieldData().asSequence().size();
			for (size_t i = 0; i < nElements; ++i) {
				auto rowSizer = std::make_shared<UISizer>();

				rowSizer->add(context.makeField(fieldType, pars.withSubIndex(i), false), 1);

				auto deleteButton = std::make_shared<UIButton>("delete" + toString(i), context.getUIFactory().getStyle("buttonThin"), LocalisedString::fromHardcodedString("-"));
				deleteButton->setMinSize(Vector2f(22, 22));
				rowSizer->add(deleteButton);

				container->add(rowSizer);
			}
			
			auto addButton = std::make_shared<UIButton>("add", context.getUIFactory().getStyle("buttonThin"), LocalisedString::fromHardcodedString("+"));
			addButton->setMinSize(Vector2f(22, 22));
			container->add(addButton);
		};
		buildList();

		container->setHandle(UIEventType::ButtonClicked, [=, buildList = std::move(buildList)] (const UIEvent& event)
		{
			auto& seq = data.getFieldData().asSequence();
			if (event.getSourceId() == "add") {
				seq.emplace_back(ConfigNode());
			} else if (event.getSourceId().startsWith("delete")) {
				const size_t index = event.getSourceId().mid(6).toInteger();
				seq.erase(seq.begin() + index);
			}
			buildList();
		});
		
		return container;
	}
};

class ComponentEditorStdSetFieldFactory : public ComponentEditorStdVectorFieldFactory {
public:
	String getFieldType() override
	{
		return "std::set<>";
	}
};

class ComponentEditorStdOptionalFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "std::optional<>";
	}

	bool isNested() const override
	{
		return false;
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode();
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		const auto fieldType = pars.typeParameters.at(0);
		const auto data = pars.data;

		const bool initialValue = pars.data.getFieldData().getType() != ConfigNodeType::Undefined;

		auto rowSizer = std::make_shared<UISizer>();
		auto checkbox = std::make_shared<UICheckbox>("present", context.getUIFactory().getStyle("checkbox"), initialValue);
		rowSizer->add(checkbox);

		auto container = std::make_shared<UIWidget>("container", Vector2f(), UISizer(UISizerType::Vertical));
		rowSizer->add(container, 1);

		auto setState = [&context, container, data, fieldType, pars] (bool newVal)
		{
			if (newVal) {
				if (data.getFieldData().getType() == ConfigNodeType::Undefined) {
					data.getFieldData() = context.getDefaultNode(fieldType);
				}
				container->add(context.makeField(fieldType, pars, false));
			} else {
				container->clear();
				data.getFieldData() = ConfigNode();
			}
			context.onEntityUpdated();
		};
		setState(initialValue);

		checkbox->bindData("present", initialValue, std::move(setState));
		
		return rowSizer;
	}
};

class ComponentEditorOptionalLiteFieldFactory : public ComponentEditorStdOptionalFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::OptionalLite<>";
	}
};

class ComponentEditorColourFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::Colour4f";
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode("#FFFFFF");
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		// TODO: make this a proper colour picker
		auto data = pars.data;
		const auto defaultValue = pars.defaultValue.isEmpty() ? "#FFFFFF" : pars.defaultValue;

		String value = data.getFieldData().asString(defaultValue);

		auto container = std::make_shared<UISizer>();
		
		auto field = std::make_shared<UITextInput>(context.getUIFactory().getKeyboard(), "colourHex", context.getUIFactory().getStyle("inputThin"), value, LocalisedString::fromUserString(defaultValue));
		container->add(field, 1);

		auto colourPreview = std::make_shared<UIImage>(Sprite().setImage(context.getUIFactory().getResources(), "halley_ui/ui_list_item.png").setColour(Colour4f::fromString(value)));
		colourPreview->setMinSize(Vector2f(40, 22));
		container->add(colourPreview);
		
		field->bindData("colourHex", value, [&context, data, colourPreview](String newVal)
		{
			colourPreview->getSprite().setColour(Colour4f::fromString(newVal));
			data.getFieldData() = ConfigNode(std::move(newVal));
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
	factories.emplace_back(std::make_unique<ComponentEditorPolygonFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorVertexListFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorStdVectorFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorStdSetFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorStdOptionalFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorOptionalLiteFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorColourFieldFactory>());

	return factories;
}
