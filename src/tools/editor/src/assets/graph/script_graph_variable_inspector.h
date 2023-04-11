#pragma once

namespace Halley {

    class ScriptGraphVariableEntry : public UIWidget
    {
    public:
        ScriptGraphVariableEntry(UIFactory& factory);

        void onMakeUI() override;
        void setData(const String& name, const String& value, const String& type, const String& scope);

    private:
        std::shared_ptr<UILabel> nameLabel;
        std::shared_ptr<UILabel> valueLabel;
        std::shared_ptr<UILabel> typeLabel;
        std::shared_ptr<UILabel> scopeLabel;
    };

    class ScriptGraphVariableInspector : public UIWidget
	{
    public:
        ScriptGraphVariableInspector(UIFactory& factory);

        void onMakeUI() override;
        void updateVariables(const ConfigNode& data);
    private:
        UIFactory& factory;
        std::shared_ptr<UIWidget> list;
    };
}