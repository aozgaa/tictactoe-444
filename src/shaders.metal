#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 normal;
};

struct Uniforms {
    float4x4 mvp;
    float4   color;
};

vertex VertexOut vert_main(VertexIn in [[stage_in]],
                           constant Uniforms &u [[buffer(1)]])
{
    VertexOut out;
    out.position = u.mvp * float4(in.position, 1.0);
    out.normal   = in.normal;
    return out;
}

fragment float4 frag_main(VertexOut in [[stage_in]],
                           constant Uniforms &u [[buffer(1)]])
{
    // Simple diffuse-like shading from a fixed light direction
    float3 light = normalize(float3(1.0, 2.0, 3.0));
    float diffuse = 0.4 + 0.6 * saturate(dot(normalize(in.normal), light));
    return float4(u.color.rgb * diffuse, u.color.a);
}
