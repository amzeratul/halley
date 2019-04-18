#include <metal_stdlib>
using namespace metal;

// Must match output of vertex shader
struct VertexOut {
  float2 texCoord0;
  float2 pixelTexCoord0;
  float4 colour;
  float4 colourAdd;
  float2 vertPos;
  float2 pixelPos;
  float4 position [[ position ]];
};

fragment float4 pixel_func (
  VertexOut v [[ stage_in ]]
) {
  return v.colour;
}
