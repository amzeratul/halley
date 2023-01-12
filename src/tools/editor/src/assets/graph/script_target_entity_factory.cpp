#include "script_target_entity_factory.h"

using namespace Halley;

ScriptTargetEntityFactory::ScriptTargetEntityFactory(ScriptGraphEditor& editor)
	: editor(editor)
{}

String ScriptTargetEntityFactory::getFieldType()
{
    return "Halley::ScriptTargetId";
}

std::shared_ptr<IUIElement> ScriptTargetEntityFactory::createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars)
{
    auto data = pars.data;

    const String value = data.getFieldData().asString("");

    Vector<String> ids;
    // TODO: populate

    const auto& dropStyle = context.getUIFactory().getStyle("dropdownLight");
    auto dropdown = std::make_shared<UIDropdown>("systemType", dropStyle);
    dropdown->setOptions(std::move(ids));

    dropdown->bindData("systemType", value, [&context, data, this](String newVal)
    {
        data.getWriteableFieldData() = ConfigNode(std::move(newVal));
        context.onEntityUpdated();
    });

    return dropdown;
}