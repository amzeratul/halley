#pragma once
#include "halley/ui/ui_factory.h"

namespace Halley {
    class EditorUIFactory : public UIFactory {
    public:
		EditorUIFactory(Resources& resources, I18N& i18n);
    };
}
