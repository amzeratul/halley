#include "halley/blit_attribute.hlsl"

BlitOut main(BlitIn input) {
    BlitOut result;
    result.position = input.position;
    result.texCoord0 = input.texCoord0.xy;
    return result;
}
