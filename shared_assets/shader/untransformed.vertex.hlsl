#include "halley/sprite_attribute.hlsl"

VOut main(VIn input)
{
    VOut output;
    output.position = float4(input.position + input.size * input.vertPos.xy, 0.0, 1.0);
    output.colour = input.colour;
    output.custom0 = input.custom0;
    return output;
}
