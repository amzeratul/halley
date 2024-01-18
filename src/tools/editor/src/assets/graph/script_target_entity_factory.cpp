#include "script_target_entity_factory.h"

#include "script_graph_editor.h"

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

    Vector<String> ids = editor.getScriptTargetIds();

    const auto& dropStyle = context.getUIFactory().getStyle("dropdownLight");
    auto dropdown = std::make_shared<UIDropdown>("scriptTarget", dropStyle);
    dropdown->setOptions(std::move(ids));

    dropdown->bindData("scriptTarget", value, [&context, data, this](String newVal)
    {
        data.getWriteableFieldData() = ConfigNode(std::move(newVal));
        context.onEntityUpdated();
    });

    return dropdown;
}
