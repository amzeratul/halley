#pragma once

#include "halley/text/halleystring.h"
#include "halley/core/graphics/sprite/sprite.h"
#include <vector>
#include <memory>

namespace Halley {
    class UIWidget;
    class Resources;
    class HalleyAPI;

    class IEditorCustomTools { 
    public:
        struct ToolData {
            String id;
            String text;
            Sprite icon;
            std::shared_ptr<UIWidget> widget;
        };

        struct MakeToolArgs {
            Resources& editorResources;
            Resources& gameResources;
            const HalleyAPI& api;

            MakeToolArgs(Resources& editorResources, Resources& gameResources, const HalleyAPI& api)
                : editorResources(editorResources)
                , gameResources(gameResources)
                , api(api)
            {}
        };

        virtual ~IEditorCustomTools() = default;

        virtual std::vector<ToolData> makeTools(const MakeToolArgs& args) = 0;
    };
}
