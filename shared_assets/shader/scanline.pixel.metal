#include <metal_stdlib>
using namespace metal;

struct MaterialBlock {
  float4 col0;
  float4 col1;
  float distance;
};

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
  VertexOut v [[ stage_in ]],
  texture2d<float> tex0 [[ texture(0) ]],
  constant MaterialBlock& material [[ buffer(0) ]]
) {
  return v.colour * mix(material.col0, material.col1, fract(v.texCoord0.y / material.distance));
}
