#pragma once

namespace Halley {
    class EditorUIFactory {
    public:
		EditorUIFactory(Resources& resources);

		std::shared_ptr<UILabel> makeLabel(const String& label);

    private:
		Resources& resources;
    	I18N i18n;
		UIStyleSheet styleSheet;
    };
}
