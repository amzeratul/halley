#pragma once
#include "graphics_enums.h"
#include "render_context.h"
#include "material/material.h"

namespace Halley {
    class RenderSnapshot {
    public:
        void start();
        void end();

        void bind(RenderContext& context);
        void unbind(RenderContext& context);
    	void clear(std::optional<Colour4f> colour, std::optional<float> depth, std::optional<uint8_t> stencil);
	    void draw(Material& material, size_t size, gsl::span<char> span, gsl::span<const IndexType> indices, PrimitiveType primitive, bool allIndicesAreQuads);
        void setClip(Rect4i rect, bool enable);
    };
}
