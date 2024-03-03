#include "halley/sprite_attribute.hlsl"
#include "halley/sprite_data_block.hlsl"

VOut main(VIn inputVertex)
{
    OIn input = objectData[inputVertex.idx];

    VOut output;
    output.position = float4(input.position + input.size * inputVertex.vertPos.xy, 0.0, 1.0);
    output.colour = input.colour;
    output.custom0 = input.custom0;
    return output;
}
