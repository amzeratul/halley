#include <metal_stdlib>
using namespace metal;

struct Uniforms {

  struct HalleyBlock {
    float4x4 mvp;
  };

  struct MaterialBlock {
    float smoothness;
    float outline;
    float4 outlineColour;
  };

  HalleyBlock HalleyBlock;
  MaterialBlock MaterialBlock;
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
  constant Uniforms& uniforms [[ buffer(0) ]]
) {
  return v.colour;
}
