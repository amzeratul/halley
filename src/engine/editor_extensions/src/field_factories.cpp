#include "field_factories.h"
#include "component_editor_context.h"
#include "component_field_parameters.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_sizer.h"
#include "halley/ui/widgets/ui_dropdown.h"

using namespace Halley;

EnumFieldFactory::EnumFieldFactory(String name, std::vector<String> values)
	: fieldName(std::move(name))
	, values(std::move(values))
{
	
}

std::shared_ptr<IUIElement> EnumFieldFactory::createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars)
{
	const auto data = pars.data;

	auto& fieldData = data.getFieldData();
	const auto& defaultValue = pars.getStringDefaultParameter();
	const auto& value = fieldData.asString(defaultValue.isEmpty() ? values.front() : defaultValue);

	const auto& dropStyle = context.getUIFactory().getStyle("dropdown");

	auto container = std::make_shared<UIWidget>(data.getName(), Vector2f(), UISizer(UISizerType::Grid, 4.0f, 2));
	auto dropdown = std::make_shared<UIDropdown>("enum", dropStyle);
	dropdown->setOptions(values);
	container->add(dropdown);

	container->bindData("enum", value, [&context, data](String newVal) {
		auto& node = data.getWriteableFieldData();
		node = ConfigNode(std::move(newVal));
		context.onEntityUpdated();
	});
	
	return container;
}

String EnumFieldFactory::getFieldType()
{
	return fieldName;
}
