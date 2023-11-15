#include "entity_editor_factories.h"
#include "halley/graphics/sprite/animation.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_validator.h"
#include "halley/ui/widgets/ui_checkbox.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "src/ui/select_asset_widget.h"
#include "halley/editor_extensions/component_field_parameters.h"
#include "halley/editor_extensions/component_editor_context.h"
#include "halley/maths/colour_gradient.h"
#include "src/assets/curve_editor_window.h"
#include "src/assets/gradient_editor.h"
#include "src/ui/colour_picker.h"

using namespace Halley;

class ComponentEditorTextFieldFactory : public IComponentEditorFieldFactory {
public:
	ComponentEditorTextFieldFactory(String fieldName)
		: fieldName(std::move(fieldName))
	{}

	String getFieldType() override
	{
		return fieldName;
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(String());
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.getStringDefaultParameter();

		String value = data.getFieldData().asString("");

		auto field = std::make_shared<UITextInput>("textValue", context.getUIFactory().getStyle("inputThin"), value, LocalisedString::fromUserString(defaultValue));
		field->bindData("textValue", value, [&context, data](String newVal)
		{
			data.getWriteableFieldData() = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		return field;
	}

private:
	String fieldName;
};

class ComponentEditorCodeEditorFactory : public IComponentEditorFieldFactory {
public:
	ComponentEditorCodeEditorFactory(String fieldName)
		: fieldName(std::move(fieldName))
	{}

	String getFieldType() override
	{
		return fieldName;
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(String());
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.getStringDefaultParameter();

		String value = data.getFieldData().asString("");

		auto field = std::make_shared<UITextInput>("textValue", context.getUIFactory().getStyle("inputThin"), value, LocalisedString::fromUserString(defaultValue));
		field->bindData("textValue", value, [&context, data](String newVal)
		{
			data.getWriteableFieldData() = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		field->setMultiLine(true);
		field->setMinSize(Vector2f(60, 100));

		return field;
	}

private:
	String fieldName;
};

class ComponentEditorIntFieldFactory : public IComponentEditorFieldFactory {
public:
	ComponentEditorIntFieldFactory(String fieldType, std::optional<float> minValue, std::optional<float> maxValue)
		: fieldType(std::move(fieldType))
		, minValue(minValue)
		, maxValue(maxValue)
	{
	}

	String getFieldType() override
	{
		return fieldType;
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(0);
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto& defaultValue = pars.getIntDefaultParameter();

		const int value = data.getFieldData().asInt(defaultValue);
		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Horizontal, 4.0f));

		auto field = std::make_shared<UISpinControl2>("intValue", context.getUIFactory().getStyle("spinControl"), float(value), false);
		field->bindData("intValue", value, [&context, data](int newVal)
		{
			data.getWriteableFieldData() = ConfigNode(newVal);
			context.onEntityUpdated();
		});
		field->setMinimumValue(minValue);
		field->setMaximumValue(maxValue);
		container->add(field, 1);

		auto reset = std::make_shared<UIButton>("resetValue", context.getUIFactory().getStyle("buttonThin"), UISizer());
		reset->setIcon(Sprite().setImage(context.getGameResources(), "entity_icons/reset.png"));
		reset->setMinSize(Vector2f(22, 22));
		reset->setToolTip(LocalisedString::fromHardcodedString("Reset to default value"));
		reset->setHandle(UIEventType::ButtonClicked, "resetValue", [=] (const UIEvent& event)
		{
			field->setValue(float(defaultValue));
		});
		container->add(reset, 0, Vector4f(-1, 0, 0, 0));

		return container;
	}

private:
	String fieldType;
	std::optional<float> minValue;
	std::optional<float> maxValue;
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
		const auto& defaultValue = pars.getFloatDefaultParameter();

		const float value = data.getFieldData().asFloat(defaultValue);
		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Horizontal, 4.0f));

		auto field = std::make_shared<UISpinControl2>("floatValue", context.getUIFactory().getStyle("spinControl"), value, true);
		field->bindData("floatValue", value, [&context, data](float newVal)
		{
			data.getWriteableFieldData() = ConfigNode(newVal);
			context.onEntityUpdated();
		});
		container->add(field, 1);

		auto reset = std::make_shared<UIButton>("resetValue", context.getUIFactory().getStyle("buttonThin"), UISizer());
		reset->setIcon(Sprite().setImage(context.getGameResources(), "entity_icons/reset.png"));
		reset->setMinSize(Vector2f(22, 22));
		reset->setToolTip(LocalisedString::fromHardcodedString("Reset to default value"));
		reset->setHandle(UIEventType::ButtonClicked, "resetValue", [=] (const UIEvent& event)
		{
			field->setValue(defaultValue);
		});
		container->add(reset, 0, Vector4f(-1, 0, 0, 0));

		return container;
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
		const auto& defaultValue = pars.getBoolDefaultParameter();

		bool value = data.getFieldData().asBool(defaultValue);

		auto field = std::make_shared<UICheckbox>("boolValue", context.getUIFactory().getStyle("checkbox"), value);
		field->bindData("boolValue", value, [&context, data](bool newVal)
		{
			data.getWriteableFieldData() = ConfigNode(newVal);
			context.onEntityUpdated();
		});

		auto sizer = std::make_shared<UISizer>(UISizerType::Horizontal, 4.0f);
		sizer->add(field);

		return sizer;
	}
};

template <typename VecType, size_t nDimensions>
class ComponentEditorVectorFieldFactory : public IComponentEditorFieldFactory {
public:
	using ScalarType = typename VecType::ScalarType;

	ComponentEditorVectorFieldFactory(String name) : name(std::move(name)) {}

	String getFieldType() override
	{
		return name;
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(VecType());
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;

		VecType defaultValue;
		for (size_t i = 0; i < nDimensions; ++i) {
			pars.getDefaultParameter(defaultValue[i], i);
		}
		VecType value = defaultValue;
		if (data.getFieldData().getType() != ConfigNodeType::Undefined) {
			value = data.getFieldData().asType<VecType>(value);
		}

		float granularity = 1;
		if (pars.options.getType() == ConfigNodeType::Map) {
			granularity = pars.options["granularity"].asFloat(1.0f);
		}

		const auto& style = context.getUIFactory().getStyle("spinControl");
		const auto& buttonStyle = context.getUIFactory().getStyle("buttonThin");

		auto dataOutput = std::make_shared<bool>(true);
		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Horizontal, 3.0f));

		std::array<std::shared_ptr<UISpinControl2>, nDimensions> values;
		for (int i = 0; i < nDimensions; ++i) {
			values[i] = std::make_shared<UISpinControl2>("value" + toString(i), style, static_cast<float>(value[i]), true);
			values[i]->setIncrement(granularity);
			container->add(values[i], 1);
			container->bindData("value" + toString(i), value[i], [&context, data, dataOutput, defaultValue, i] (ScalarType newVal)
			{
				if (*dataOutput) {
					auto& node = data.getWriteableFieldData();
					VecType val = node.asType<VecType>(defaultValue);
					val[i] = newVal;
					node = val;
					context.onEntityUpdated();
				}
			});
		}
		
		auto reset = std::make_shared<UIButton>("resetValue", buttonStyle, UISizer());
		reset->setIcon(Sprite().setImage(context.getGameResources(), "entity_icons/reset.png"));
		reset->setMinSize(Vector2f(22, 22));
		reset->setToolTip(LocalisedString::fromHardcodedString("Reset to default value"));
		reset->setHandle(UIEventType::ButtonClicked, "resetValue", [=] (const UIEvent& event)
		{
			for (size_t i = 0; i < nDimensions; ++i) {
				values[i]->setValue(static_cast<float>(defaultValue[i]));
			}
		});
		container->add(reset, 0, Vector4f(-1, 0, 0, 0));

		container->setHandle(UIEventType::ReloadData, pars.componentName + ":" + data.getName(), [=] (const UIEvent& event)
		{
			VecType newVal;
			if (data.getFieldData().getType() != ConfigNodeType::Undefined) {
				newVal = data.getFieldData().asType<VecType>();
			}
			*dataOutput = false;
			for (size_t i = 0; i < nDimensions; ++i) {
				event.getCurWidget().getWidgetAs<UITextInput>("value" + toString(i))->setText(toString(newVal[i]));
			}
			*dataOutput = true;
		});

		return container;
	}

private:
	String name;
};

class ComponentEditorVertexFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::Vertex";
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(ConfigNode::SequenceType());
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		const auto data = pars.data;
		const auto componentName = pars.componentName;

		auto style = context.getUIFactory().getStyle("buttonThin");

		auto field = std::make_shared<UIButton>("editVertex", style, LocalisedString::fromHardcodedString("Edit..."));
		field->setMinSize(Vector2f(30, 22));

		field->setHandle(UIEventType::ButtonClicked, "editVertex", [=, &context] (const UIEvent& event)
		{
			context.setTool("vertex", componentName, data.getName());
		});

		return field;
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
		const auto& data = pars.data;

		auto& fieldData = data.getWriteableFieldData(); // HACK
		fieldData.ensureType(ConfigNodeType::Map);

		auto materialWidget = std::make_shared<SelectAssetWidget>("material", context.getUIFactory(), AssetType::MaterialDefinition, context.getGameResources(), context.getProjectWindow());
		materialWidget->setDefaultAssetId(MaterialDefinition::defaultMaterial);
		
		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2));
		container->getSizer().setColumnProportions({{0, 1}});
		container->add(context.makeLabel("Material"));
		container->add(materialWidget);
		container->add(context.makeLabel("Colour"));
		container->add(context.makeField("Halley::Colour4f", pars.withSubKey("colour", "#FFFFFF"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Pivot"));
		container->add(context.makeField("std::optional<Halley::Vector2f>", pars.withSubKey("pivot"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Flip"));
		container->add(context.makeField("bool", pars.withSubKey("flip"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Visible"));
		container->add(context.makeField("bool", pars.withSubKey("visible", "true"), ComponentEditorLabelCreation::Never));
		auto containerWeak = std::weak_ptr<UIWidget>(container);

		auto prevMaterialParameters = std::make_shared<Vector<std::shared_ptr<IUIElement>>>();
		auto addMaterialParameters = [&context, containerWeak, prevMaterialParameters, pars](const String& materialName)
		{
			const auto& data = pars.data;
			auto container = containerWeak.lock();
			if (!container) {
				return;
			}

			gsl::span<const MaterialTexture> textures;
			gsl::span<const MaterialUniformBlock> uniformBlocks;
			MaterialTexture dummy;
			if (!materialName.isEmpty() && context.getGameResources().exists<MaterialDefinition>(materialName)) {
				const auto material = context.getGameResources().get<MaterialDefinition>(materialName);
				textures = material->getTextures();
				uniformBlocks = material->getUniformBlocks();
			} else {
				dummy.name = "image";
				textures = gsl::span(&dummy, 1);
			}
			
			for (auto& widget: *prevMaterialParameters) {
				container->remove(*widget);
			}
			prevMaterialParameters->clear();
			
			size_t insertPos = 2;
			size_t i = 0;
			for (const auto& tex: textures) {
				String key = "tex_" + tex.name;
				bool isPrimary = i == 0;

				auto& fieldData = data.getFieldData();
				const String backupKey = i == 0 ? "image" : (i == 1 ? "image1" : "");
				const String& srcData = fieldData[fieldData.hasKey(key) || backupKey.isEmpty() ? key : backupKey].asString("");

				const auto label = context.makeLabel("- " + tex.name);
				const auto widget = std::make_shared<SelectAssetWidget>(key, context.getUIFactory(), AssetType::Sprite, context.getGameResources(), context.getProjectWindow());
				widget->setDefaultAssetId(tex.defaultTextureName);
				
				container->add(label, 0, Vector4f(), UISizerFillFlags::Fill, Vector2f(), insertPos++);
				container->add(widget, 0, Vector4f(), UISizerFillFlags::Fill, Vector2f(), insertPos++);
				prevMaterialParameters->push_back(label);
				prevMaterialParameters->push_back(widget);

				container->bindData(key, srcData, [&context, data, containerWeak, key, isPrimary](String newVal)
				{
					if (isPrimary) {
						context.setDefaultName(filterName(newVal), filterName(data.getFieldData()[key].asString("")));

						auto material = containerWeak.lock()->getWidgetAs<SelectAssetWidget>("material");
						if (material->getValue().isEmpty()) {
							const auto materialName = context.getGameResources().get<SpriteResource>(newVal)->getDefaultMaterialName();

							// Important: run this on main thread. Otherwise, this call will result in addTextures being called again, invalidating this method halfway through its execution.
							Concurrent::execute(Executors::getMainUpdateThread(), [materialName, material]() {
								material->setValue(materialName);
							});
						}
					}

					data.getWriteableFieldData()[key] = newVal.isEmpty() ? ConfigNode() : ConfigNode(std::move(newVal));
					context.onEntityUpdated();
				});
				
				i++;
			}

			for (const auto& block: uniformBlocks) {
				for (const auto& uniform: block.uniforms) {
					if (!uniform.editable) {
						continue;
					}
					
					String key = "par_" + uniform.name;
					String type;
					ConfigNode options = ConfigNode::MapType();

					if (uniform.type == ShaderParameterType::Float || uniform.type == ShaderParameterType::Int) {
						String typeName = uniform.type == ShaderParameterType::Float ? "float" : "int";
						if (uniform.range) {
							type = "Halley::Range<" + typeName + ">";
							options["start"] = uniform.range->start;
							options["end"] = uniform.range->end;
						} else {
							type = typeName;
						}
					} else if (uniform.type == ShaderParameterType::Float2) {
						type = "Halley::Vector2f";
					} else if (uniform.type == ShaderParameterType::Int2) {
						type = "Halley::Vector2i";
					}
					options["granularity"] = uniform.granularity;

					if (type.isEmpty()) {
						continue;
					}

					Vector<String> defaultValue;
					if (uniform.defaultValue.getType() != ConfigNodeType::Undefined) {
						if (uniform.defaultValue.getType() == ConfigNodeType::Sequence) {
							defaultValue = uniform.defaultValue.asVector<String>({});
						} else {
							defaultValue.push_back(uniform.defaultValue.asString());
						}
					}

					const auto label = context.makeLabel("- " + uniform.name);
					const auto widget = context.makeField(type, pars.withSubKey(key, defaultValue).withOptions(std::move(options)), ComponentEditorLabelCreation::Never);
					
					container->add(label, 0, Vector4f(), UISizerFillFlags::Fill, Vector2f(), insertPos++);
					container->add(widget, 0, Vector4f(), UISizerFillFlags::Fill, Vector2f(), insertPos++);
					prevMaterialParameters->push_back(label);
					prevMaterialParameters->push_back(widget);

					auto& fieldData = data.getFieldData();
					container->bindData(key, fieldData[key].asString(""), [&context, data, containerWeak, key](String newVal)
					{
						data.getWriteableFieldData()[key] = newVal.isEmpty() ? ConfigNode() : ConfigNode(std::move(newVal));
						context.onEntityUpdated();
					});
				}
			}
		};
		
		container->bindData("material", fieldData["material"].asString(""), [&context, data, addMaterialParameters](String newVal)
		{
			addMaterialParameters(newVal);
			data.getWriteableFieldData()["material"] = newVal.isEmpty() ? ConfigNode() : ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});
		addMaterialParameters(fieldData["material"].asString(""));

		return container;
	}

	static String filterName(const String& name)
	{
		auto filename = Path(name).getFilename().toString();
		if (filename.contains(':')) {
			const auto pos = filename.find_last_of(':');
			return filename.mid(pos + 1);
		}
		return filename;
	}
};

class ComponentEditorTextRendererFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::TextRenderer";
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
		const auto& data = pars.data;

		auto& fieldData = data.getWriteableFieldData(); // HACK
		fieldData.ensureType(ConfigNodeType::Map);

		auto fontWidget = std::make_shared<SelectAssetWidget>("font", context.getUIFactory(), AssetType::Font, context.getGameResources(), context.getProjectWindow());
		fontWidget->setDefaultAssetId("Ubuntu Bold");
		
		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2));
		container->getSizer().setColumnProportions({{0, 1}});
		container->add(context.makeLabel("Text"));
		container->add(context.makeField("Halley::String", pars.withSubKey("text", ""), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Font"));
		container->add(fontWidget);
		container->add(context.makeLabel("Size"));
		container->add(context.makeField("float", pars.withSubKey("size", "20"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Outline"));
		container->add(context.makeField("float", pars.withSubKey("outline", "0"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Colour"));
		container->add(context.makeField("Halley::Colour4f", pars.withSubKey("colour", "#000000"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Outline Colour"));
		container->add(context.makeField("Halley::Colour4f", pars.withSubKey("outlineColour", "#000000"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Alignment"));
		container->add(context.makeField("float", pars.withSubKey("alignment", "0"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Offset"));
		container->add(context.makeField("Halley::Vector2f", pars.withSubKey("offset"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Smoothness"));
		container->add(context.makeField("float", pars.withSubKey("smoothness"), ComponentEditorLabelCreation::Never));
		// TODO: clip, pixel offset, colour override, line spacing

		container->bindData("font", fieldData["font"].asString(""), [&context, data](String newVal)
		{
			data.getWriteableFieldData()["font"] = ConfigNode(std::move(newVal));
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

		auto& fieldData = data.getWriteableFieldData(); // HACK
		fieldData.ensureType(ConfigNodeType::Map);

		auto& resources = context.getGameResources();
		const auto& dropStyle = context.getUIFactory().getStyle("dropdownLight");

		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2));
		container->getSizer().setColumnProportions({{0, 1}});
		container->add(context.makeLabel("Animation"));
		container->add(std::make_shared<SelectAssetWidget>("animation", context.getUIFactory(), AssetType::Animation, context.getGameResources(), context.getProjectWindow()));
		container->add(context.makeLabel("Material"));
		auto material = std::make_shared<UITextInput>("material", context.getUIFactory().getStyle("inputThin"), "");
		material->setReadOnly(true);
		material->setEnabled(false);
		container->add(material);
		container->add(context.makeLabel("Sequence"));
		container->add(std::make_shared<UIDropdown>("sequence", dropStyle));
		container->add(context.makeLabel("Direction"));
		container->add(std::make_shared<UIDropdown>("direction", dropStyle));
		container->add(context.makeLabel("Playback Speed"));
		container->add(context.makeField("float", pars.withSubKey("playbackSpeed", "1" ), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Apply Pivot"));
		container->add(context.makeField("bool", pars.withSubKey("applyPivot", "true" ), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Apply Material"));
		container->add(context.makeField("bool", pars.withSubKey("applyMaterial", "true" ), ComponentEditorLabelCreation::Never));

		auto updateAnimation = [container, data, &resources] (const String& animName)
		{
			Vector<String> sequences;
			Vector<String> directions;

			auto material = container->getWidgetAs<UITextInput>("material");
			if (animName.isEmpty()) {
				material->setGhostText(LocalisedString());
			} else {
				const auto anim = resources.get<Animation>(animName);
				directions = anim->getDirectionNames();
				sequences = anim->getSequenceNames();

				auto mat = anim->getMaterial();
				if (mat) {
					material->setGhostText(LocalisedString::fromUserString(mat->getDefinition().getName()));
				} else {
					material->setGhostText(LocalisedString());
				}
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
			data.getWriteableFieldData()["animation"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("sequence", fieldData["sequence"].asString(""), [&context, data](String newVal)
		{
			data.getWriteableFieldData()["sequence"] = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		container->bindData("direction", fieldData["direction"].asString(""), [&context, data](String newVal)
		{
			data.getWriteableFieldData()["direction"] = ConfigNode(std::move(newVal));
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
		const auto data = pars.data;
		const auto componentName = pars.componentName;

		auto style = context.getUIFactory().getStyle("buttonThin");

		auto listToString = [] (gsl::span<const Vector2f> points) -> String
		{
			String result = "[";
			bool first = true;
			for (auto& p: points) {
				if (!first) {
					result += ", ";
				}
				first = false;
				result += toString(p);
			}
			result += "]";
			return result;
		};

		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Horizontal, 4.0f));

		String value = listToString(data.getFieldData().asVector<Vector2f>());
		auto field = std::make_shared<UITextInput>("textValue", context.getUIFactory().getStyle("inputThin"), value);
		field->setReadOnly(true);
		container->add(field, 1);
		
		auto button = std::make_shared<UIButton>("editPolygon", style, LocalisedString::fromHardcodedString("Edit..."));
		button->setMinSize(Vector2f(30, 22));
		container->add(button, 0);

       	container->setHandle(UIEventType::ReloadData, pars.componentName + ":" + data.getName(), [=](const UIEvent& event) {
			Vector<Vector2f> newVal;
			if (data.getFieldData().getType() != ConfigNodeType::Undefined) {
				newVal = data.getFieldData().asVector<Vector2f>();
			}
			event.getCurWidget().getWidgetAs<UITextInput>("textValue")->setText(listToString(newVal));
		});

		container->setHandle(UIEventType::ButtonClicked, "editPolygon", [=, &context] (const UIEvent& event)
		{
			context.setTool("polygon", componentName, data.getName());
		});

		return container;
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
		return "Halley::Vector<>";
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

		auto& fieldData = data.getWriteableFieldData(); // HACK
		if (fieldData.getType() != ConfigNodeType::Sequence) {
			if (fieldData.getType() == ConfigNodeType::Map) {
				fieldData.ensureType(ConfigNodeType::Sequence);
			}
			else if (fieldData.getType() == ConfigNodeType::Undefined) {
				fieldData = ConfigNode::SequenceType();
			}
			else {
				fieldData = ConfigNode::SequenceType({ std::move(fieldData) });
			}
		}
		
		const auto containerPtr = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Vertical));
		const auto containerWeak = std::weak_ptr<UIWidget>(containerPtr);

		auto buildList = [=, &context] () {
			const auto container = containerWeak.lock();
			container->clear();

			const size_t nElements = data.getFieldData().asSequence().size();
			
			for (size_t i = 0; i < nElements; ++i) {
				auto rowSizer = std::make_shared<UISizer>();

				rowSizer->add(context.makeField(fieldType, pars.withSubIndex(i), ComponentEditorLabelCreation::OnlyIfNested), 1);

				auto deleteButton = std::make_shared<UIButton>("delete" + toString(i), context.getUIFactory().getStyle("buttonThin"), LocalisedString::fromHardcodedString("-"));
				deleteButton->setMinSize(Vector2f(22, 22));
				deleteButton->setToolTip(LocalisedString::fromHardcodedString("Remove entry"));
				rowSizer->add(deleteButton);

				container->add(rowSizer);
			}
			
			auto addButton = std::make_shared<UIButton>("add", context.getUIFactory().getStyle("buttonThin"), LocalisedString::fromHardcodedString("+"));
			addButton->setMinSize(Vector2f(22, 22));
			addButton->setToolTip(LocalisedString::fromHardcodedString("Add new entry on " + pars.data.getName()));
			container->add(addButton);
		};
		buildList();

		containerPtr->setHandle(UIEventType::ButtonClicked, [=, buildList = std::move(buildList)] (const UIEvent& event)
		{
			auto& seq = data.getWriteableFieldData().asSequence();
			if (event.getSourceId() == "add") {
				seq.emplace_back(ConfigNode());
				context.onEntityUpdated();
			} else if (event.getSourceId().startsWith("delete")) {
				const size_t index = event.getSourceId().mid(6).toInteger();
				seq.erase(seq.begin() + index);
				context.onEntityUpdated();
			}
			buildList();
		});
		
		return containerPtr;
	}
};

class ComponentEditorStdPairFieldFactory : public IComponentEditorFieldFactory
{
public:
	String getFieldType() override
	{
		return "std::pair<>";
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
		const auto data = pars.data;

		auto& fieldData = data.getWriteableFieldData(); // HACK
		if (fieldData.getType() != ConfigNodeType::Sequence) {
			if (fieldData.getType() == ConfigNodeType::Map) {
				fieldData.ensureType(ConfigNodeType::Sequence);
			}
			else if (fieldData.getType() == ConfigNodeType::Undefined) {
				fieldData = ConfigNode::SequenceType();
			}
			else {
				fieldData = ConfigNode::SequenceType({ std::move(fieldData) });
			}
		}

		ConfigNode::SequenceType fieldDataSequence = fieldData.asSequence();
		while (fieldDataSequence.size() < 2) {
			auto defaultNode = context.getDefaultNode(pars.typeParameters.at(fieldDataSequence.size()));
			fieldDataSequence.emplace_back(std::move(defaultNode));
		}
		fieldData = std::move(fieldDataSequence);
		
		const auto containerPtr = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Vertical));

		containerPtr->add(context.makeField(String(pars.typeParameters.at(0)).trimBoth(), pars.withSubIndex(0), ComponentEditorLabelCreation::OnlyIfNested), 1);
		containerPtr->add(context.makeField(String(pars.typeParameters.at(1)).trimBoth(), pars.withSubIndex(1), ComponentEditorLabelCreation::OnlyIfNested), 1);
		
		return containerPtr;
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
		rowSizer->add(checkbox, 0, {}, UISizerAlignFlags::CentreHorizontal);

		auto container = std::make_shared<UIWidget>("container", Vector2f(), UISizer(UISizerType::Vertical));
		rowSizer->add(container, 1);

		auto setState = [&context, container, data, fieldType, pars] (bool newVal)
		{
			if (newVal) {
				if (data.getFieldData().getType() == ConfigNodeType::Undefined) {
					data.getWriteableFieldData() = context.getDefaultNode(fieldType);
				}
				container->add(context.makeField(fieldType, pars, ComponentEditorLabelCreation::OnlyIfNested));
			} else {
				container->clear();
				data.getWriteableFieldData() = ConfigNode();
			}
		};
		setState(initialValue);

		checkbox->bindData("present", initialValue, [&context, setState = std::move(setState)] (bool newVal)
		{
			setState(newVal);
			context.onEntityUpdated();
		});
		
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
		return ConfigNode("#000000");
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		auto defaultValue = pars.getStringDefaultParameter();
		if (defaultValue.isEmpty()) {
			defaultValue = "#000000";
		}

		String value = data.getFieldData().asString(defaultValue);

		auto container = std::make_shared<UISizer>();

		auto colourPreview = std::make_shared<ColourPickerButton>(context.getUIFactory(), value, allowNamedColour(), [&context, data](String colour, bool final)
		{
			data.getWriteableFieldData() = ConfigNode(colour);
			context.onEntityUpdated(!final);
		});
		container->add(colourPreview, 1);

		return container;
	}

	virtual bool allowNamedColour() const
	{
		return false;
	}
};

class ComponentEditorUIColourFieldFactory : public ComponentEditorColourFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::UIColour";
	}

	bool allowNamedColour() const override
	{
		return true;
	}
};

class ComponentEditorInterpolationCurveFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::InterpolationCurve";
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(1.0f);
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		auto container = std::make_shared<UISizer>();

		auto button = std::make_shared<CurveEditorButton>(context.getUIFactory(), InterpolationCurve(data.getFieldData(), false), [&context, data](InterpolationCurve curve)
		{
			data.getWriteableFieldData() = curve.toConfigNode();
			context.onEntityUpdated();
		});
		container->add(button, 1);

		return container;
	}
};

class ComponentEditorColourGradientFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::ColourGradient";
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(1.0f);
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		auto container = std::make_shared<UISizer>();

		auto button = std::make_shared<GradientEditorButton>(context.getUIFactory(), *context.getAPI().video, ColourGradient(data.getFieldData()), [&context, data](ColourGradient curve)
		{
			data.getWriteableFieldData() = curve.toConfigNode();
			context.onEntityUpdated();
		});
		container->add(button, 1);

		return container;
	}
};

class ComponentEditorParticlesFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::Particles";
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

		auto& fieldData = data.getWriteableFieldData(); // HACK
		fieldData.ensureType(ConfigNodeType::Map);

		convertLegacy(fieldData);

		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Vertical));

		auto spawnGroup = context.makeNestedField("Spawning");
		auto initialGroup = context.makeNestedField("Initial State");
		auto dynamicsGroup = context.makeNestedField("Dynamics");
		auto systemGroup = context.makeNestedField("System");
		auto multiSystemGroup = context.makeNestedField("Secondary Particles");
		auto spawnContainer = std::make_shared<UIWidget>("", Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2, {{0, 1}}));
		auto initialContainer = std::make_shared<UIWidget>("", Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2, {{0, 1}}));
		auto dynamicsContainer = std::make_shared<UIWidget>("", Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2, {{0, 1}}));
		auto systemContainer = std::make_shared<UIWidget>("", Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2, {{0, 1}}));
		auto multiSystemContainer = std::make_shared<UIWidget>("", Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2, {{0, 1}}));
		spawnGroup->getWidget("fields")->add(spawnContainer, 1);
		initialGroup->getWidget("fields")->add(initialContainer, 1);
		dynamicsGroup->getWidget("fields")->add(dynamicsContainer, 1);
		systemGroup->getWidget("fields")->add(systemContainer, 1);
		multiSystemGroup->getWidget("fields")->add(multiSystemContainer, 1);

		spawnContainer->add(context.makeLabel("Particles/s"));
		spawnContainer->add(context.makeField("float", pars.withSubKey("spawnRate", "100"), ComponentEditorLabelCreation::Never));
		spawnContainer->add(context.makeLabel("Area Shape"));
		spawnContainer->add(context.makeField("Halley::ParticleSpawnAreaShape", pars.withSubKey("spawnAreaShape"), ComponentEditorLabelCreation::Never));
		spawnContainer->add(context.makeLabel("Area Size"));
		spawnContainer->add(context.makeField("Halley::Vector2f", pars.withSubKey("spawnArea"), ComponentEditorLabelCreation::Never));
		spawnContainer->add(context.makeLabel("Max Particles"));
		spawnContainer->add(context.makeField("std::optional<int>", pars.withSubKey("maxParticles", ""), ComponentEditorLabelCreation::Never));
		spawnContainer->add(context.makeLabel("Burst"));
		spawnContainer->add(context.makeField("std::optional<int>", pars.withSubKey("burst", ""), ComponentEditorLabelCreation::Never));
		initialContainer->add(context.makeLabel("Height"));
		initialContainer->add(context.makeField("float", pars.withSubKey("startHeight", "0"), ComponentEditorLabelCreation::Never));
		initialContainer->add(context.makeLabel("Scale"));
		initialContainer->add(context.makeField("Halley::Vector2f", pars.withSubKey("initialScale", {"1", "1"}), ComponentEditorLabelCreation::Never));
		initialContainer->add(context.makeLabel("Speed"));
		initialContainer->add(context.makeField("Halley::Range<float>", pars.withSubKey("speed", { "100", "100" }), ComponentEditorLabelCreation::Never));
		initialContainer->add(context.makeLabel("Velocity Mult"));
		initialContainer->add(context.makeField("Halley::Vector3f", pars.withSubKey("velScale", {"1", "1", "1"}), ComponentEditorLabelCreation::Never));
		initialContainer->add(context.makeLabel("Azimuth"));
		initialContainer->add(context.makeField("Halley::Range<float>", pars.withSubKey("azimuth", {"0", "0"}), ComponentEditorLabelCreation::Never));
		initialContainer->add(context.makeLabel("Altitude"));
		initialContainer->add(context.makeField("Halley::Range<float>", pars.withSubKey("altitude", {"0", "0"}), ComponentEditorLabelCreation::Never));
		dynamicsContainer->add(context.makeLabel("Lifetime"));
		dynamicsContainer->add(context.makeField("Halley::Range<float>", pars.withSubKey("ttl", { "1", "1" }), ComponentEditorLabelCreation::Never));
		dynamicsContainer->add(context.makeLabel("Colour"));
		dynamicsContainer->add(context.makeField("Halley::ColourGradient", pars.withSubKey("colourGradient", ""), ComponentEditorLabelCreation::Never));
		dynamicsContainer->add(context.makeLabel("Scale"));
		dynamicsContainer->add(context.makeField("Halley::InterpolationCurve", pars.withSubKey("scaleCurve", "1"), ComponentEditorLabelCreation::Never));
		dynamicsContainer->add(context.makeLabel("Acceleration"));
		dynamicsContainer->add(context.makeField("Halley::Vector3f", pars.withSubKey("acceleration", ""), ComponentEditorLabelCreation::Never));
		dynamicsContainer->add(context.makeLabel("Speed Damp"));
		dynamicsContainer->add(context.makeField("float", pars.withSubKey("speedDamp", "0"), ComponentEditorLabelCreation::Never));
		dynamicsContainer->add(context.makeLabel("Stop Time"));
		dynamicsContainer->add(context.makeField("float", pars.withSubKey("stopTime", "0"), ComponentEditorLabelCreation::Never));
		dynamicsContainer->add(context.makeLabel("Direction Scatter"));
		dynamicsContainer->add(context.makeField("float", pars.withSubKey("directionScatter", "0"), ComponentEditorLabelCreation::Never));
		dynamicsContainer->add(context.makeLabel("Rotate Towards Movement"));
		dynamicsContainer->add(context.makeField("bool", pars.withSubKey("rotateTowardsMovement", "false"), ComponentEditorLabelCreation::Never));
		dynamicsContainer->add(context.makeLabel("Minimum Height"));
		dynamicsContainer->add(context.makeField("std::optional<float>", pars.withSubKey("minHeight", ""), ComponentEditorLabelCreation::Never));
		systemContainer->add(context.makeLabel("Destroy When Done"));
		systemContainer->add(context.makeField("bool", pars.withSubKey("destroyWhenDone", "false"), ComponentEditorLabelCreation::Never));
		multiSystemContainer->add(context.makeLabel("On Spawn"));
		multiSystemContainer->add(context.makeField("Halley::EntityId", pars.withSubKey("onSpawn", ""), ComponentEditorLabelCreation::Never));
		multiSystemContainer->add(context.makeLabel("On Death"));
		multiSystemContainer->add(context.makeField("Halley::EntityId", pars.withSubKey("onDeath", ""), ComponentEditorLabelCreation::Never));

		container->add(std::move(spawnGroup));
		container->add(std::move(initialGroup));
		container->add(std::move(dynamicsGroup));
		container->add(std::move(systemGroup));
		container->add(std::move(multiSystemGroup));

		return container;
	}

	void convertLegacy(ConfigNode& node)
	{
		if (node.hasKey("ttlScatter")) {
			const auto legacyTtl = node["ttl"].asFloat(1.0f);
			const auto ttlScatter = node["ttlScatter"].asFloat(0.2f);
			auto ttl = Range<float>(legacyTtl - ttlScatter, legacyTtl + ttlScatter);
			ttl.start = std::max(ttl.start, 0.1f);
			ttl.end = std::max(ttl.start, ttl.end);
			node["ttl"] = ttl;
			node.removeKey("ttlScatter");
		}

		if (node.hasKey("speedScatter")) {
			const auto legacySpeed = node["speed"].asFloat(100.0f);
			const auto speedScatter = node["speedScatter"].asFloat(0.0f);
			node["speed"] = Range<float>(legacySpeed - speedScatter, legacySpeed + speedScatter);
			node.removeKey("speedScatter");
		}

		if (node.hasKey("angle")) {
			const auto angle = node["angle"].asVector2f(Vector2f());
			const auto angleScatter = node["angleScatter"].asVector2f(Vector2f());
			node["azimuth"] = Range<float>(angle.x - angleScatter.x, angle.x + angleScatter.x);
			node["altitude"] = Range<float>(angle.y - angleScatter.y, angle.y + angleScatter.y);
			node.removeKey("angle");
			node.removeKey("angleScatter");
		}

		if (node.hasKey("startScale") || node.hasKey("endScale")) {
			const auto startScale = node["startScale"].asFloat(1.0f);
			const auto endScale = node["endScale"].asFloat(1.0f);
			const float scale = std::max(startScale, endScale);
			InterpolationCurve scaleCurve;
			scaleCurve.points[0].y = startScale / scale;
			scaleCurve.points[1].y = endScale / scale;
			scaleCurve.scale = scale;
			node.removeKey("startScale");
			node.removeKey("endScale");
			node["scaleCurve"] = scaleCurve.toConfigNode();
		}

		if (node.hasKey("fadeInTime") || node.hasKey("fadeOutTime")) {
			const auto ttl = node["ttl"].asFloatRange({});
			const auto fadeInTime = node["fadeInTime"].asFloat(0.0f);
			const auto fadeOutTime = node["fadeOutTime"].asFloat(0.0f);
			const float avgTTL = std::max((ttl.start + ttl.end) * 0.5f, 0.1f);
			const auto colourGradient = ColourGradient(fadeInTime / avgTTL, 1.0f - fadeOutTime / avgTTL);
			node.removeKey("fadeInTime");
			node.removeKey("fadeOutTime");
			node["colourGradient"] = colourGradient.toConfigNode();
		}
	}

};

class ComponentEditorResourceReferenceFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::ResourceReference<>";
	}

	bool isNested() const override
	{
		return false;
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode();
	}

	std::optional<AssetType> getType(String typeName)
	{
		const String strippedTypeName = typeName.startsWith("Halley::") ? typeName.mid(8) : typeName;
		if (strippedTypeName == "AudioClip") {
			return AssetType::AudioClip;
		} else if (strippedTypeName == "AudioEvent") {
			return AssetType::AudioEvent;
		} else if (strippedTypeName == "SpriteResource") {
			return AssetType::Sprite;
		} else if (strippedTypeName == "Animation") {
			return AssetType::Animation;
		} else if (strippedTypeName == "MaterialDefinition") {
			return AssetType::MaterialDefinition;
		} else if (strippedTypeName == "Prefab") {
			return AssetType::Prefab;
		} else if (strippedTypeName == "Scene") {
			return AssetType::Scene;
		} else if (strippedTypeName == "ScriptGraph") {
			return AssetType::ScriptGraph;
		} else if (strippedTypeName == "UIDefinition") {
			return AssetType::UIDefinition;
		} else if (strippedTypeName == "Texture") {
			return AssetType::Texture;
		} else {
			Logger::logWarning("Unimplemented resource type on ComponentEditorResourceReferenceFieldFactory: " + strippedTypeName);
		}

		return {};
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		const auto fieldType = pars.typeParameters.at(0);
		const auto data = pars.data;
		auto& fieldData = data.getFieldData();
		const auto& assetName = fieldData.hasKey("asset") ? fieldData["asset"].asString("") : (fieldData.getType() == ConfigNodeType::String ? fieldData.asString("") : "");
		const auto& defaultValue = pars.getStringDefaultParameter();
		
		const std::optional<AssetType> type = getType(fieldType);

		std::shared_ptr<IUIElement> result;
		if (type) {
			auto widget = std::make_shared<SelectAssetWidget>("asset", context.getUIFactory(), type.value(), context.getGameResources(), context.getProjectWindow());
			widget->bindData("asset", assetName, [&context, data](String newVal)
			{
				auto& fieldData = data.getWriteableFieldData();
				fieldData = ConfigNode(std::move(newVal)); 
				context.onEntityUpdated();
			});
			widget->setDefaultAssetId(defaultValue);
			result = widget;
		} else {
			result = context.makeLabel("N/A");
		}
		
		return result;
	}
};

class ComponentEditorUIStyleFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::UIStyle<>";
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
		const auto uiClass = pars.typeParameters.at(0);
		const auto data = pars.data;
		auto& fieldData = data.getFieldData();
		const auto& styleName = fieldData.asString("");
		const auto& defaultValue = pars.getStringDefaultParameter();
		
		auto widget = std::make_shared<SelectUIStyleWidget>("style", context.getUIFactory(), uiClass, context.getGameResources(), context.getProjectWindow());
		widget->bindData("style", styleName, [&context, data](String newVal)
		{
			auto& fieldData = data.getWriteableFieldData();
			fieldData = ConfigNode(std::move(newVal)); 
			context.onEntityUpdated();
		});
		widget->setDefaultAssetId(defaultValue);

		return widget;
	}
};

class ComponentEditorScriptGraphFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::ScriptGraph";
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		const auto data = pars.data;
		const auto componentName = pars.componentName;

		auto style = context.getUIFactory().getStyle("buttonThin");

		auto field = std::make_shared<UIButton>("editScript", style, LocalisedString::fromHardcodedString("Edit..."));
		field->setMinSize(Vector2f(30, 22));

		field->setHandle(UIEventType::ButtonClicked, "editScript", [=, &context] (const UIEvent& event)
		{
			context.setTool("!scripting", componentName, data.getName());
		});

		return field;
	}
};

class ComponentEditorRangeFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::Range<>";
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		if (pars.typeParameters.size() >= 3 && pars.typeParameters.size() <= 4) {
			const auto range = Range<float>(pars.typeParameters[1].toFloat(), pars.typeParameters[2].toFloat());
			float granularity = 0.0f;
			if (pars.typeParameters.size() == 4) {
				granularity = pars.typeParameters[3].toFloat();
			}
			return makeSlider(context, pars, range, granularity);
		} else if (pars.options.getType() == ConfigNodeType::Map) {
			Range<float> range;
			range.start = pars.options["start"].asFloat(0.0f);
			range.end = pars.options["end"].asFloat(1.0f);
			const float granularity = pars.options["granularity"].asFloat(0.0f);
			return makeSlider(context, pars, range, granularity);
		} else {
			const bool intType = !pars.typeParameters.empty() && pars.typeParameters[0] == "int";
			return context.makeField(intType ? "Halley::Vector2i" : "Halley::Vector2f", pars, ComponentEditorLabelCreation::Never);
		}
	}

	std::shared_ptr<IUIElement> makeSlider(const ComponentEditorContext& context, const ComponentFieldParameters& pars, Range<float> range, float granularity)
	{
		const auto data = pars.data;
		const bool intType = !pars.typeParameters.empty() && pars.typeParameters[0] == "int";

		auto style = context.getUIFactory().getStyle("slider");
		auto field = std::make_shared<UISlider>("range", style, range.start, range.end, 0.0f, true, !intType);

		if (intType) {
			field->setGranularity(1.0f);
		} else {
			int decimalPlaces = 2;
			float baseGranularity = 0.01f;

			const float totalRange = range.getLength();
			if (totalRange >= 100) {
				baseGranularity = 1.0f;
				decimalPlaces = 0;
			} else if (totalRange >= 10) {
				baseGranularity = 0.1f;
				decimalPlaces = 1;
			}

			if (granularity < 0.00001f) {
				granularity = baseGranularity;
			}
			field->setGranularity(granularity);
			field->setLabelConversion([decimalPlaces](float v) { return LocalisedString::fromUserString(toString(v, decimalPlaces)); });
		}
		field->setMinSize(Vector2f(30, 22));

		if (intType) {
			const auto& defaultValue = pars.getIntDefaultParameter();
			const int value = data.getFieldData().asInt(defaultValue);
			field->bindData("range", value, [&context, data](int newVal)
			{
				data.getWriteableFieldData() = ConfigNode(newVal);
				context.onEntityUpdated();
			});
		} else {
			const auto& defaultValue = pars.getFloatDefaultParameter();
			const float value = data.getFieldData().asFloat(defaultValue);
			field->bindData("range", value, [&context, data](float newVal)
			{
				data.getWriteableFieldData() = ConfigNode(newVal);
				context.onEntityUpdated();
			});
		}

		return field;
	}
};

class ComponentEditorUIAlignFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::UISizerAlignFlags::Type";
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;
		const auto defaultValue = UIFactory::parseSizerAlignFlags(ConfigNode(pars.defaultValue));
		const auto value = UIFactory::parseSizerAlignFlags(data.getFieldData(), defaultValue);

		auto& res = context.getGameResources();

		auto fillHorizontal = std::make_shared<UICheckbox>("fillHorizontal", context.getUIFactory().getStyle("checkbox"), false);
		auto fillVertical = std::make_shared<UICheckbox>("fillVertical", context.getUIFactory().getStyle("checkbox"), false);
		const auto fillHWeak = std::weak_ptr<UICheckbox>(fillHorizontal);
		const auto fillVWeak = std::weak_ptr<UICheckbox>(fillVertical);

		auto fillSizer = std::make_shared<UISizer>(UISizerType::Grid, 1.0f, 2);
		fillSizer->add(std::move(fillHorizontal), 0, {}, UISizerAlignFlags::Centre);
		fillSizer->add(std::make_shared<UIImage>(Sprite().setImage(res, "arrows/arrow_left_right.png")), 0, {}, UISizerAlignFlags::Centre);
		fillSizer->add(std::move(fillVertical), 0, {}, UISizerAlignFlags::Centre);
		fillSizer->add(std::make_shared<UIImage>(Sprite().setImage(res, "arrows/arrow_up_down.png")), 0, {}, UISizerAlignFlags::Centre);

		Vector<String> listIds;
		auto alignList = std::make_shared<UIList>("align", context.getUIFactory().getStyle("list"), UISizerType::Grid, 3);
		auto addDir = [&] (int dir, std::string_view imageName)
		{
			auto id = toString(dir);
			listIds.push_back(id);
			auto img = std::make_shared<UIImage>(Sprite().setImage(res, imageName));
			img->setDisablable(Colour4f(1, 1, 1, 1), Colour4f(1, 1, 1, 0.5f));
			alignList->addImage(id, img, 0, {}, UISizerAlignFlags::Centre);
		};
		addDir(UISizerAlignFlags::Top | UISizerAlignFlags::Left, "arrows/arrow_top_left.png");
		addDir(UISizerAlignFlags::Top | UISizerAlignFlags::CentreHorizontal, "arrows/arrow_top.png");
		addDir(UISizerAlignFlags::Top | UISizerAlignFlags::Right, "arrows/arrow_top_right.png");
		addDir(UISizerAlignFlags::CentreVertical | UISizerAlignFlags::Left, "arrows/arrow_left.png");
		addDir(UISizerAlignFlags::CentreVertical | UISizerAlignFlags::CentreHorizontal, "arrows/arrow_centre.png");
		addDir(UISizerAlignFlags::CentreVertical | UISizerAlignFlags::Right, "arrows/arrow_right.png");
		addDir(UISizerAlignFlags::Bottom | UISizerAlignFlags::Left, "arrows/arrow_bottom_left.png");
		addDir(UISizerAlignFlags::Bottom | UISizerAlignFlags::CentreHorizontal, "arrows/arrow_bottom.png");
		addDir(UISizerAlignFlags::Bottom | UISizerAlignFlags::Right, "arrows/arrow_bottom_right.png");

		const auto alignWeak = std::weak_ptr<UIList>(alignList);

		auto topWidget = std::make_shared<UIWidget>("alignWidget", Vector2f{}, UISizer(UISizerType::Horizontal, 10.0f));
		topWidget->add(std::move(fillSizer), 0, {}, UISizerAlignFlags::Centre);
		topWidget->add(std::move(alignList), 0, {}, UISizerAlignFlags::Centre);

		auto updateValue = [=] ()
		{
			const auto& alignCurSel = alignWeak.lock()->getSelectedOptionId();
			int value = alignCurSel.isInteger() ? alignCurSel.toInteger() : 0;

			if (fillHWeak.lock()->isChecked()) {
				value = (value & ~(UISizerAlignFlags::Left | UISizerAlignFlags::CentreHorizontal | UISizerAlignFlags::Right)) | UISizerAlignFlags::FillHorizontal;
			}
			if (fillVWeak.lock()->isChecked()) {
				value = (value & ~(UISizerAlignFlags::Top | UISizerAlignFlags::CentreVertical | UISizerAlignFlags::Bottom)) | UISizerAlignFlags::FillVertical;
			}

			data.getWriteableFieldData() = UIFactory::makeSizerAlignFlagsNode(UISizerAlignFlags::Type(value));
			context.onEntityUpdated();
		};

		auto updateAlignList = [=] ()
		{
			const int horizontalFlags = UISizerAlignFlags::Left | UISizerAlignFlags::Right | UISizerAlignFlags::CentreHorizontal;
			const int verticalFlags = UISizerAlignFlags::Top | UISizerAlignFlags::Bottom | UISizerAlignFlags::CentreVertical;
			int valid = 0;
			if (!fillHWeak.lock()->isChecked()) {
				valid |= horizontalFlags;
			} else {
				valid |= UISizerAlignFlags::CentreHorizontal;
			}
			if (!fillVWeak.lock()->isChecked()) {
				valid |= verticalFlags;
			} else {
				valid |= UISizerAlignFlags::CentreVertical;
			}

			auto list = alignWeak.lock();

			auto prevSel = list->getSelectedOptionId();
			std::optional<int> newSelection;
			for (const auto& idStr: listIds) {
				const auto id = idStr.toInteger();
				const bool selected = idStr == prevSel;
				const bool enabled = (id & valid) == id;
				list->setItemEnabled(idStr, enabled);
				if (selected && !enabled) {
					auto newTarget = id & valid;
					if ((newTarget & horizontalFlags) == 0) {
						newTarget |= UISizerAlignFlags::CentreHorizontal;
					}
					if ((newTarget & verticalFlags) == 0) {
						newTarget |= UISizerAlignFlags::CentreVertical;
					}
					newSelection = newTarget;
				}
			}
			auto curSel = list->getSelectedOptionId();

			if (newSelection) {
				list->setSelectedOptionId(toString(newSelection.value()));
			}
		};

		topWidget->bindData("fillHorizontal", (value & UISizerAlignFlags::FillHorizontal) != 0, [=] (bool newValue)
		{
			updateAlignList();
			updateValue();
		});

		topWidget->bindData("fillVertical", (value & UISizerAlignFlags::FillVertical) != 0, [=] (bool newValue)
		{
			updateAlignList();
			updateValue();
		});

		topWidget->bindData("align", toString(int(UIFactory::normalizeDirection(value, true))), [=] (String newValue)
		{
			updateValue();
		});

		updateAlignList();

		return topWidget;
	}
};



class ComponentEditorScriptMessageTypeFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::ScriptMessageType";
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

		auto& fieldData = data.getWriteableFieldData(); // HACK
		fieldData.ensureType(ConfigNodeType::Map);

		auto& resources = context.getGameResources();
		const auto& dropStyle = context.getUIFactory().getStyle("dropdownLight");

		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2));
		container->getSizer().setColumnProportions({{0, 1}});
		container->add(context.makeLabel("Script"));
		container->add(std::make_shared<SelectAssetWidget>("script", context.getUIFactory(), AssetType::ScriptGraph, context.getGameResources(), context.getProjectWindow()));
		container->add(context.makeLabel("Message"));
		container->add(std::make_shared<UIDropdown>("messageId", dropStyle));

		auto updateMessage = [&context, &resources, data](String newVal)
		{
			int nParams = 0;
			const auto scriptName = data.getFieldData()["script"].asString("");
			if (!scriptName.isEmpty()) {
				const auto script = resources.get<ScriptGraph>(scriptName);
				nParams = script->getMessageNumParams(newVal);
			}

			data.getWriteableFieldData()["message"] = ConfigNode(std::move(newVal));
			data.getWriteableFieldData()["nParams"] = ConfigNode(nParams);
			context.onEntityUpdated();
		};

		auto updateScript = [container, data, &resources, updateMessage] (const String& scriptName)
		{
			Vector<String> messages;

			if (!scriptName.isEmpty()) {
				const auto script = resources.get<ScriptGraph>(scriptName);
				messages = script->getMessageNames();
			}

			auto message = container->getWidgetAs<UIDropdown>("messageId");
			message->setOptions(messages);
			message->setSelectedOption(data.getFieldData()["message"].asString(""));
			updateMessage(message->getSelectedOptionId());
		};

		container->bindData("messageId", fieldData["message"].asString(""), updateMessage);
		
		updateScript(fieldData["script"].asString(""));

		container->bindData("script", fieldData["script"].asString(""), [&context, data, updateScript](String newVal)
		{
			data.getWriteableFieldData()["script"] = ConfigNode(newVal);
			updateScript(newVal);
			context.onEntityUpdated();
		});

		return container;
	}
};



class ComponentEditorEntityMessageTypeFieldFactory : public IComponentEditorFieldFactory {
public:
	ComponentEditorEntityMessageTypeFieldFactory(const ECSData& ecsData, String fieldType, bool systemMessage)
		: ecsData(ecsData)
		, fieldType(std::move(fieldType))
		, systemMessage(systemMessage)
	{}
	
	String getFieldType() override
	{
		return fieldType;
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(ConfigNode::MapType());
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;

		auto& fieldData = data.getWriteableFieldData(); // HACK
		fieldData.ensureType(ConfigNodeType::Map);

		Vector<String> messageIds;
		if (systemMessage) {
			for (auto& [k, v]: ecsData.getSystemMessages()) {
				if (v.serializable) {
					messageIds.push_back(k);
				}
			}
		} else {
			for (auto& [k, v]: ecsData.getMessages()) {
				if (v.serializable) {
					messageIds.push_back(k);
				}
			}
		}
		std::sort(messageIds.begin(), messageIds.end());

		const auto& dropStyle = context.getUIFactory().getStyle("dropdownLight");
		auto dropdown = std::make_shared<UIDropdown>("messageType", dropStyle);
		dropdown->setOptions(std::move(messageIds));
		dropdown->setSelectedOption(data.getFieldData()["message"].asString(""));

		dropdown->bindData("messageType", fieldData["message"].asString(""), [&context, data, this](String newVal)
		{
			data.getWriteableFieldData() = getMessageConfig(newVal);
			context.onEntityUpdated();
		});

		return dropdown;
	}

private:
	const ECSData& ecsData;
	String fieldType;
	bool systemMessage;

	ConfigNode getMessageConfig(const String& messageId) const
	{
		ConfigNode::MapType result;
		result["message"] = messageId;

		if (const auto* msg = getMessage(messageId)) {
			ConfigNode::SequenceType members;
			members.reserve(msg->members.size());
			for (const auto& m: msg->members) {
				members.push_back(ConfigNode(m.name));
			}
			result["members"] = std::move(members);

			if (systemMessage) {
				if (auto* sysMsg = static_cast<const SystemMessageSchema*>(msg)) {
					result["returnType"] = sysMsg->returnType;
				}
			}
		}
		
		return result;
	}

	const MessageSchema* getMessage(const String& messageId) const
	{
		if (systemMessage) {
			const auto& msgs = ecsData.getSystemMessages();
			const auto iter = msgs.find(messageId);
			if (iter != msgs.end()) {
				return &(iter->second);
			}
		} else {
			const auto& msgs = ecsData.getMessages();
			const auto iter = msgs.find(messageId);
			if (iter != msgs.end()) {
				return &(iter->second);
			}
		}
		return nullptr;
	}
};


class ComponentEditorSystemFieldFactory : public IComponentEditorFieldFactory {
public:
	ComponentEditorSystemFieldFactory(const ECSData& ecsData)
		: ecsData(ecsData)
	{}
	
	String getFieldType() override
	{
		return "Halley::System";
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(ConfigNode::MapType());
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;

		const String value = data.getFieldData().asString("");

		Vector<String> systemIds;
		for (auto& [k, v]: ecsData.getSystems()) {
			systemIds.push_back(k);
		}
		std::sort(systemIds.begin(), systemIds.end());

		const auto& dropStyle = context.getUIFactory().getStyle("dropdownLight");
		auto dropdown = std::make_shared<UIDropdown>("systemType", dropStyle);
		dropdown->setOptions(std::move(systemIds));

		dropdown->bindData("systemType", value, [&context, data, this](String newVal)
		{
			data.getWriteableFieldData() = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

		return dropdown;
	}

private:
	const ECSData& ecsData;
};

class ComponentEditorComponentFieldFactory : public IComponentEditorFieldFactory {
public:
	ComponentEditorComponentFieldFactory(const ECSData& ecsData)
		: ecsData(ecsData)
	{}
	
	String getFieldType() override
	{
		return "Halley::ScriptComponentFieldType";
	}

	ConfigNode getDefaultNode() const override
	{
		return ConfigNode(ConfigNode::MapType());
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		auto data = pars.data;

		const auto value = ScriptComponentFieldType(data.getFieldData());

		Vector<String> componentIds;
		for (auto& [k, v]: ecsData.getComponents()) {
			componentIds.push_back(k);
		}
		std::sort(componentIds.begin(), componentIds.end());

		const auto& dropStyle = context.getUIFactory().getStyle("dropdownLight");

		auto container = std::make_shared<UIWidget>("", Vector2f(), UISizer(UISizerType::Vertical));

		auto componentDropdown = std::make_shared<UIDropdown>("componentType", dropStyle);
		componentDropdown->setOptions(std::move(componentIds));
		container->add(componentDropdown);

		auto fieldDropdown = std::make_shared<UIDropdown>("fieldName", dropStyle);
		container->add(fieldDropdown);

		auto& ecs = ecsData;
		auto populateFields = [fieldDropdown, &ecs, data](const String& componentId) -> String
		{
			Vector<String> fieldIds;
			const auto iter = ecs.getComponents().find(componentId);
			if (iter != ecs.getComponents().end()) {
				for (const auto& member: iter->second.members) {
					if (std_ex::contains(member.serializationTypes, EntitySerialization::Type::Dynamic)) {
						fieldIds.push_back(member.name);
					}
				}
				std::sort(fieldIds.begin(), fieldIds.end());
			}

			const auto value = ScriptComponentFieldType(data.getFieldData());
			int defaultOption = -1;
			const auto iter2 = std::find(fieldIds.begin(), fieldIds.end(), value.field);
			if (iter2 != fieldIds.end()) {
				defaultOption = static_cast<int>(iter2 - fieldIds.begin());
			}

			fieldDropdown->setOptions(std::move(fieldIds), defaultOption);
			fieldDropdown->setSelectedOption(value.field);
			return fieldDropdown->getSelectedOptionId();
		};

		componentDropdown->bindData("componentType", value.component, [&context, data, this, populateFields](String newVal)
		{
			auto type = ScriptComponentFieldType(data.getFieldData());
			type.component = std::move(newVal);
			type.field = populateFields(type.component);
			data.getWriteableFieldData() = type.toConfig();
			context.onEntityUpdated();
		});

		populateFields(componentDropdown->getSelectedOptionId());
		fieldDropdown->bindData("fieldName", value.field, [&context, data, this](String newVal)
		{
			auto type = ScriptComponentFieldType(data.getFieldData());
			type.field = std::move(newVal);
			data.getWriteableFieldData() = type.toConfig();
			context.onEntityUpdated();
		});

		return container;
	}

private:
	const ECSData& ecsData;
};

class ComponentEditorParsedOptionFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::UIFactory::ParsedOption";
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
		const auto& data = pars.data;

		auto& fieldData = data.getWriteableFieldData(); // HACK
		fieldData.ensureType(ConfigNodeType::Map);
		
		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2));
		container->getSizer().setColumnProportions({{0, 1}});
		container->add(context.makeLabel("Id"));
		container->add(context.makeField("Halley::String", pars.withSubKey("id", ""), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Text"));
		container->add(context.makeField("Halley::String", pars.withSubKey("text", ""), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Text (Key)"));
		container->add(context.makeField("Halley::String", pars.withSubKey("textKey", ""), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Tooltip"));
		container->add(context.makeField("Halley::String", pars.withSubKey("tooltip", ""), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Tooltip (Key)"));
		container->add(context.makeField("Halley::String", pars.withSubKey("tooltipKey", ""), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Image"));
		container->add(context.makeField("Halley::ResourceReference<Halley::SpriteResource>", pars.withSubKey("image", ""), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Image (Inactive)"));
		container->add(context.makeField("Halley::ResourceReference<Halley::SpriteResource>", pars.withSubKey("inactiveImage", ""), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Image Colour"));
		container->add(context.makeField("Halley::Colour4f", pars.withSubKey("imageColour", ""), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Border"));
		container->add(context.makeField("Halley::Vector4f", pars.withSubKey("border", ""), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Active"));
		container->add(context.makeField("bool", pars.withSubKey("active", ""), ComponentEditorLabelCreation::Never));
		
		return container;
	}
};

class ComponentEditorEntityIdFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::EntityId";
	}

	std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override
	{
		const auto data = pars.data;
		auto& fieldData = data.getFieldData();
		const auto& uuid = fieldData.asString("");
		
		auto widget = std::make_shared<SelectEntityWidget>("entity", context.getUIFactory(), context.getProjectWindow(), context.getEntityEditorCallbacks());
		widget->bindData("entity", uuid, [&context, data](String newVal)
		{
			auto& fieldData = data.getWriteableFieldData();
			fieldData = ConfigNode(std::move(newVal)); 
			context.onEntityUpdated();
		});

		return widget;
	}
};


Vector<std::unique_ptr<IComponentEditorFieldFactory>> EntityEditorFactories::getDefaultFactories()
{
	Vector<std::unique_ptr<IComponentEditorFieldFactory>> factories;

	factories.emplace_back(std::make_unique<ComponentEditorTextFieldFactory>("Halley::String"));
	factories.emplace_back(std::make_unique<ComponentEditorTextFieldFactory>("Halley::ScriptTargetId"));
	factories.emplace_back(std::make_unique<ComponentEditorCodeEditorFactory>("Halley::LuaExpression"));
	factories.emplace_back(std::make_unique<ComponentEditorIntFieldFactory>("int8_t", static_cast<float>(std::numeric_limits<int8_t>::min()), static_cast<float>(std::numeric_limits<int8_t>::max())));
	factories.emplace_back(std::make_unique<ComponentEditorIntFieldFactory>("int16_t", static_cast<float>(std::numeric_limits<int16_t>::min()), static_cast<float>(std::numeric_limits<int16_t>::max())));
	factories.emplace_back(std::make_unique<ComponentEditorIntFieldFactory>("int", std::nullopt, std::nullopt));
	factories.emplace_back(std::make_unique<ComponentEditorIntFieldFactory>("int32_t", std::nullopt, std::nullopt));
	factories.emplace_back(std::make_unique<ComponentEditorIntFieldFactory>("int64_t", std::nullopt, std::nullopt));
	factories.emplace_back(std::make_unique<ComponentEditorIntFieldFactory>("uint8_t", 0.0f, static_cast<float>(std::numeric_limits<uint8_t>::max())));
	factories.emplace_back(std::make_unique<ComponentEditorIntFieldFactory>("uint16_t", 0.0f, static_cast<float>(std::numeric_limits<uint16_t>::max())));
	factories.emplace_back(std::make_unique<ComponentEditorIntFieldFactory>("uint32_t", 0.0f, std::nullopt));
	factories.emplace_back(std::make_unique<ComponentEditorIntFieldFactory>("uint64_t", 0.0f, std::nullopt));
	factories.emplace_back(std::make_unique<ComponentEditorFloatFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorAngle1fFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorBoolFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorVectorFieldFactory<Vector2i, 2>>("Halley::Vector2i"));
	factories.emplace_back(std::make_unique<ComponentEditorVectorFieldFactory<Vector2f, 2>>("Halley::Vector2f"));
	factories.emplace_back(std::make_unique<ComponentEditorVectorFieldFactory<Vector3i, 3>>("Halley::Vector3i"));
	factories.emplace_back(std::make_unique<ComponentEditorVectorFieldFactory<Vector3f, 3>>("Halley::Vector3f"));
	factories.emplace_back(std::make_unique<ComponentEditorVectorFieldFactory<Vector4f, 4>>("Halley::Vector4f"));
	factories.emplace_back(std::make_unique<ComponentEditorVectorFieldFactory<Vector4i, 4>>("Halley::Vector4i"));
	factories.emplace_back(std::make_unique<ComponentEditorVertexFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorSpriteFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorTextRendererFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorAnimationPlayerFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorPolygonFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorVertexListFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorStdVectorFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorStdPairFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorStdSetFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorStdOptionalFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorOptionalLiteFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorColourFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorUIColourFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorColourGradientFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorInterpolationCurveFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorParticlesFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorResourceReferenceFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorUIStyleFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorScriptGraphFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorRangeFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorUIAlignFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorScriptMessageTypeFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorParsedOptionFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorEntityIdFieldFactory>());

	factories.emplace_back(EnumFieldFactory::makeEnumFactory<DefaultInputButtons>("Halley::InputButton"));
	factories.emplace_back(EnumFieldFactory::makeEnumFactory<InputPriority>("Halley::InputPriority"));
	factories.emplace_back(EnumFieldFactory::makeEnumFactory<UISizerType>("Halley::UISizerType"));
	factories.emplace_back(EnumFieldFactory::makeEnumFactory<UIScrollDirection>("Halley::UIScrollDirection"));
	factories.emplace_back(EnumFieldFactory::makeEnumFactory<ScriptVariableScope>("Halley::ScriptVariableScope"));
	factories.emplace_back(EnumFieldFactory::makeEnumFactory<MathOp>("Halley::MathOp"));
	factories.emplace_back(EnumFieldFactory::makeEnumFactory<MathRelOp>("Halley::MathRelOp"));
	factories.emplace_back(EnumFieldFactory::makeEnumFactory<TweenCurve>("Halley::TweenCurve"));
	factories.emplace_back(EnumFieldFactory::makeEnumFactory<LoggerLevel>("Halley::LoggerLevel"));
	factories.emplace_back(EnumFieldFactory::makeEnumFactory<ParticleSpawnAreaShape>("Halley::ParticleSpawnAreaShape"));
	factories.emplace_back(EnumFieldFactory::makeEnumFactory<AssetType>("Halley::AssetType"));

	return factories;
}

Vector<std::unique_ptr<IComponentEditorFieldFactory>> EntityEditorFactories::getECSFactories(const ECSData& ecsData)
{
	Vector<std::unique_ptr<IComponentEditorFieldFactory>> factories;

	factories.emplace_back(std::make_unique<ComponentEditorEntityMessageTypeFieldFactory>(ecsData, "Halley::EntityMessageType", false));
	factories.emplace_back(std::make_unique<ComponentEditorEntityMessageTypeFieldFactory>(ecsData, "Halley::SystemMessageType", true));
	factories.emplace_back(std::make_unique<ComponentEditorSystemFieldFactory>(ecsData));
	factories.emplace_back(std::make_unique<ComponentEditorComponentFieldFactory>(ecsData));
	return factories;
}
