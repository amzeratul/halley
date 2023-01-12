#pragma once

namespace Halley {
	class ScriptGraphEditor;

	class ScriptTargetEntityFactory : public IComponentEditorFieldFactory {
    public:
        ScriptTargetEntityFactory(ScriptGraphEditor& editor);        
        String getFieldType() override;
        std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars) override;

	private:
        ScriptGraphEditor& editor;
    };
}
