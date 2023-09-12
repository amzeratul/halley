#include "halley/sprite_attribute.hlsl"
#include "halley/sprite_basic_vertex.hlsl"
#include "halley/text.hlsl"

VOut main(VIn input) {
    input.position += u_shadowDistance.xy;
    VOut result;
    basicVertex(input, result, true);
    return result;
}
