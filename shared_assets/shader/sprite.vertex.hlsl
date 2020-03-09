#include "halley/sprite_attribute.hlsl"
#include "halley/sprite_basic_vertex.hlsl"

VOut main(VIn input) {
    VOut result;
    basicVertex(input, result);
    return result;
}
