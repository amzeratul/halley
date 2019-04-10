#include <metal_stdlib>
using namespace metal;

struct PixelIn {
  float2 v_texCoord0;
  float4 v_colour;
  float4 v_colourAdd;
};

struct Uniforms {
  float4 u_col0;
  float4 u_col1;
  float u_distance;
};

fragment float4 fragment_pixel(VertexOut vert [[stage_in]]) {
  return float4(0, 0, 0, 0);
}
