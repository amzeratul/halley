#pragma once
#include "halley/graph/base_graph_enums.h"

namespace Halley {
    enum class RenderGraphElementType {
		Unknown = uint8_t(BaseGraphNodeElementType::Undefined),
		Node = uint8_t(BaseGraphNodeElementType::Node),
        ColourBuffer,
        DepthStencilBuffer,
        Texture,
        Dependency
    };
}
