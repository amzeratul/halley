#include "halley/sprite_attribute.hlsl"
#include "halley/sprite_basic_vertex.hlsl"
#include "halley/text_vertex_block.hlsl"

VOut main(VIn input) {
    VOut result;
    basicVertexOffset(input, result, true, u_shadowDistance.xy);
    return result;
}
