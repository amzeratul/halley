#include <metal_stdlib>
using namespace metal;

struct Uniforms {

  struct HalleyBlock {
    float4x4 mvp;
  };

  HalleyBlock HalleyBlock;

};

struct VertexIn {
  float4 vertPos;
  float2 position;
  float2 pivot;
  float2 size;
  float2 scale;
  float4 colour;
  float4 texCoord0;
  float rotation;
  float textureRotation;
};

struct VertexOut {
  float2 texCoord0;
  float2 pixelTexCoord0;
  float4 colour;
  float4 colourAdd;
  float2 vertPos;
  float2 pixelPos;
  float4 position [[ position ]];
};

float2 getTexCoord(float4 texCoords, float2 vertPos, float texCoordRotation) {
  float2 texPos = mix(vertPos, float2(1.0 - vertPos.y, vertPos.x), texCoordRotation);
  return float2(mix(texCoords.xy, texCoords.zw, texPos.xy));
}

struct ComputedColours {
  float4 baseColour;
  float4 addColour;
};

ComputedColours getColours(float4 inColour) {
  auto out = ComputedColours();
  // Premultiply alpha
  float4 premult = float4(inColour.rgb * inColour.a, inColour.a);
  out.baseColour = clamp(premult, float4(0, 0, 0, 0), float4(1, 1, 1, 1));
  out.addColour = clamp(premult - out.baseColour, float4(0, 0, 0, 0), float4(1, 1, 1, 1));
  return out;
}

float4 getVertexPosition(float4x4 mvp, float2 position, float2 pivot, float2 size, float2 vertPos, float angle) {
  float c = cos(angle);
  float s = sin(angle);
  float2x2 m = float2x2(c, s, -s, c);

  float2 pos = position + m * ((vertPos - pivot) * size);
  return /*mvp * */ float4(pos, 0, 1);
}

vertex VertexOut vertex_func (
  const device VertexIn* vertex_array [[ buffer(0) ]],
  constant Uniforms& uniforms [[ buffer(1) ]],
  unsigned int vid [[ vertex_id ]]
) {
  VertexIn v = vertex_array[vid];

  VertexOut outVertex = VertexOut();
  outVertex.texCoord0 = getTexCoord(v.texCoord0, v.vertPos.zw, v.textureRotation);
  outVertex.pixelTexCoord0 = outVertex.texCoord0 * v.size;
  outVertex.vertPos = v.vertPos.xy;

  auto colours = getColours(v.colour);
  outVertex.colour = colours.baseColour;
  outVertex.colourAdd = colours.addColour;

  outVertex.position = getVertexPosition(
    uniforms.HalleyBlock.mvp, v.position, v.pivot,
    v.size * v.scale, v.vertPos.xy, v.rotation
  );

  return outVertex;
}
