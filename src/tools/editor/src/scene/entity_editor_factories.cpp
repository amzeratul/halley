#include "entity_editor_factories.h"
#include "halley/core/graphics/sprite/animation.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_validator.h"
#include "halley/ui/widgets/ui_checkbox.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "src/ui/select_asset_widget.h"
#include "halley/editor_extensions/component_field_parameters.h"
#include "halley/editor_extensions/component_editor_context.h"

using namespace Halley;

class ComponentEditorTextFieldFactory : public IComponentEditorFieldFactory {
public:
	String getFieldType() override
	{
		return "Halley::String";
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
		const auto& defaultValue = pars.getIntDefaultParameter();

		const int value = data.getFieldData().asInt(defaultValue);
		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Horizontal, 4.0f));

		auto field = std::make_shared<UISpinControl2>("intValue", context.getUIFactory().getStyle("spinControl"), float(value), false);
		field->bindData("intValue", value, [&context, data](int newVal)
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
			field->setValue(float(defaultValue));
		});
		container->add(reset, 0, Vector4f(-1, 0, 0, 0));

		return container;
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

		const auto& style = context.getUIFactory().getStyle("spinControl");
		const auto& buttonStyle = context.getUIFactory().getStyle("buttonThin");

		auto dataOutput = std::make_shared<bool>(true);
		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Horizontal, 3.0f));

		std::array<std::shared_ptr<UISpinControl2>, nDimensions> values;
		for (int i = 0; i < nDimensions; ++i) {
			values[i] = std::make_shared<UISpinControl2>("value" + toString(i), style, static_cast<float>(value[i]), true);
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
		materialWidget->setDefaultAssetId("Halley/Sprite");
		
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

		auto prevMaterialParameters = std::make_shared<std::vector<std::shared_ptr<IUIElement>>>();
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
							Concurrent::execute(Executors::getMainThread(), [materialName, material]() {
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

					if (uniform.type == ShaderParameterType::Float || uniform.type == ShaderParameterType::Int) {
						String typeName = uniform.type == ShaderParameterType::Float ? "float" : "int";
						if (uniform.range) {
							type = "Halley::Range<" + typeName + "," + toString(uniform.range->start) + "," + toString(uniform.range->end) + ">";
						} else {
							type = typeName;
						}
					} else if (uniform.type == ShaderParameterType::Float2) {
						type = "Halley::Vector2f";
					} else if (uniform.type == ShaderParameterType::Int2) {
						type = "Halley::Vector2i";
					}

					if (type.isEmpty()) {
						continue;
					}
					
					const auto label = context.makeLabel("- " + uniform.name);
					const auto widget = context.makeField(type, pars.withSubKey(key), ComponentEditorLabelCreation::Never);
					
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
			std::vector<String> sequences;
			std::vector<String> directions;

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
			std::vector<Vector2f> newVal;
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
		// TODO: make this a proper colour picker
		auto data = pars.data;
		auto defaultValue = pars.getStringDefaultParameter();
		if (defaultValue.isEmpty()) {
			defaultValue = "#000000";
		}

		String value = data.getFieldData().asString(defaultValue);

		auto container = std::make_shared<UISizer>();
		
		auto field = std::make_shared<UITextInput>("colourHex", context.getUIFactory().getStyle("inputThin"), value, LocalisedString::fromUserString(defaultValue));
		container->add(field, 1);

		auto colourPreview = std::make_shared<UIImage>(Sprite().setImage(context.getUIFactory().getResources(), "halley_ui/ui_list_item.png").setColour(Colour4f::fromString(value)));
		colourPreview->setMinSize(Vector2f(40, 22));
		container->add(colourPreview);
		
		field->bindData("colourHex", value, [&context, data, colourPreview](String newVal)
		{
			colourPreview->getSprite().setColour(Colour4f::fromString(newVal));
			data.getWriteableFieldData() = ConfigNode(std::move(newVal));
			context.onEntityUpdated();
		});

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

		auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2));
		container->getSizer().setColumnProportions({{0, 1}});
		container->add(context.makeLabel("Spawn Rate"));
		container->add(context.makeField("float", pars.withSubKey("spawnRate", "100"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Spawn Area"));
		container->add(context.makeField("Halley::Vector2f", pars.withSubKey("spawnArea"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Max Particles"));
		container->add(context.makeField("std::optional<int>", pars.withSubKey("maxParticles", ""), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Burst"));
		container->add(context.makeField("std::optional<int>", pars.withSubKey("burst", ""), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("TTL"));
		container->add(context.makeField("float", pars.withSubKey("ttl", "1"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("TTL Scatter"));
		container->add(context.makeField("float", pars.withSubKey("ttlScatter", "0.2"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Speed"));
		container->add(context.makeField("float", pars.withSubKey("speed", "100"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Speed Scatter"));
		container->add(context.makeField("float", pars.withSubKey("speedScatter", "10"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Speed Damp"));
		container->add(context.makeField("float", pars.withSubKey("speedDamp", "0"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Acceleration"));
		container->add(context.makeField("Halley::Vector2f", pars.withSubKey("acceleration", ""), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Angle"));
		container->add(context.makeField("float", pars.withSubKey("angle", "0"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Angle Scatter"));
		container->add(context.makeField("float", pars.withSubKey("angleScatter", "10"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Start Scale"));
		container->add(context.makeField("float", pars.withSubKey("startScale", "1"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("End Scale"));
		container->add(context.makeField("float", pars.withSubKey("endScale", "1"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Fade-in Time"));
		container->add(context.makeField("float", pars.withSubKey("fadeInTime", "0"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Fade-out Time"));
		container->add(context.makeField("float", pars.withSubKey("fadeOutTime", "0"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Stop Time"));
		container->add(context.makeField("float", pars.withSubKey("stopTime", "0"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Direction Scatter"));
		container->add(context.makeField("float", pars.withSubKey("directionScatter", "0"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Rotate Towards Movement"));
		container->add(context.makeField("bool", pars.withSubKey("rotateTowardsMovement", "false"), ComponentEditorLabelCreation::Never));
		container->add(context.makeLabel("Destroy When Done"));
		container->add(context.makeField("bool", pars.withSubKey("destroyWhenDone", "false"), ComponentEditorLabelCreation::Never));
		
		auto containerWeak = std::weak_ptr<UIWidget>(container);

		return container;
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
			context.setTool("scripting", componentName, data.getName());
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
		const auto data = pars.data;
		const auto componentName = pars.componentName;

		const bool intType = !pars.typeParameters.empty() && pars.typeParameters[0] == "int";

		auto style = context.getUIFactory().getStyle("slider");

		Range<float> range(0, 1);
		if (pars.typeParameters.size() == 3) {
			range = Range<float>(pars.typeParameters[1].toFloat(), pars.typeParameters[2].toFloat());
		}
		auto field = std::make_shared<UISlider>("range", style, range.start, range.end);
		if (intType) {
			field->setGranularity(1.0f);
		} else {
			const float totalRange = range.getLength();
			float granularity = 0.01f;
			int decimalPlaces = 2;
			if (totalRange >= 100) {
				granularity = 1.0f;
				decimalPlaces = 0;
			} else if (totalRange >= 10) {
				granularity = 0.1f;
				decimalPlaces = 1;
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

		std::vector<String> listIds;
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
			int value = 0;
			if (fillHWeak.lock()->isChecked()) {
				value |= UISizerAlignFlags::FillHorizontal;
			}
			if (fillVWeak.lock()->isChecked()) {
				value |= UISizerAlignFlags::FillVertical;
			}
			const auto& alignCurSel = alignWeak.lock()->getSelectedOptionId();
			value |= alignCurSel.isInteger() ? alignCurSel.toInteger() : 0;

			data.getWriteableFieldData() = UIFactory::makeSizerAlignFlagsNode(UISizerAlignFlags::Type(value));
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
			
			std::optional<int> newSelection;
			for (const auto& idStr: listIds) {
				const auto item = list->getItem(idStr);
				const auto id = idStr.toInteger();
				const bool selected = item->isSelected();
				const bool enabled = (id & valid) == id;
				item->setEnabled(enabled);
				if (selected && !enabled) {
					newSelection = id & valid;
				}
			}
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

		topWidget->bindData("align", toString(int(value)), [=] (String newValue)
		{
			//updateValue();
		});

		updateAlignList();

		return topWidget;
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
	factories.emplace_back(std::make_unique<ComponentEditorVectorFieldFactory<Vector2i, 2>>("Halley::Vector2i"));
	factories.emplace_back(std::make_unique<ComponentEditorVectorFieldFactory<Vector2f, 2>>("Halley::Vector2f"));
	factories.emplace_back(std::make_unique<ComponentEditorVectorFieldFactory<Vector3i, 3>>("Halley::Vector3i"));
	factories.emplace_back(std::make_unique<ComponentEditorVectorFieldFactory<Vector3f, 3>>("Halley::Vector3f"));
	factories.emplace_back(std::make_unique<ComponentEditorVectorFieldFactory<Vector4f, 4>>("Halley::Vector4f"));
	factories.emplace_back(std::make_unique<ComponentEditorVectorFieldFactory<Vector4i, 4>>("Halley::Vector4i"));
	factories.emplace_back(std::make_unique<ComponentEditorVertexFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorSpriteFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorAnimationPlayerFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorPolygonFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorVertexListFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorStdVectorFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorStdPairFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorStdSetFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorStdOptionalFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorOptionalLiteFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorColourFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorParticlesFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorResourceReferenceFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorScriptGraphFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorRangeFieldFactory>());
	factories.emplace_back(std::make_unique<ComponentEditorUIAlignFactory>());

	factories.emplace_back(EnumFieldFactory::makeEnumFactory<UISizerType>("Halley::UISizerType"));

	return factories;
}
