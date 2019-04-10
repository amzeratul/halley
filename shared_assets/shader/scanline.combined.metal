#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    packed_float3 position;
    packed_float4 color;
};

struct VertexOut {
  float4 computedPosition [[position]];
  float4 color;
};

vertex VertexOut vertex_func( // 1
  const device VertexIn* vertex_array [[ buffer(0) ]],
  unsigned int vid [[ vertex_id ]]) {
    VertexIn v = vertex_array[vid];

    VertexOut outVertex = VertexOut();
    outVertex.computedPosition = float4(v.position, 1.0);
    outVertex.color = v.color;
    return outVertex;
}

fragment float4 pixel_func(VertexOut interpolated [[stage_in]]) {
  return float4(interpolated.color);
}
