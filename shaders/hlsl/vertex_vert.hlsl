struct VSInput
{
    [[vk::location(0)]] float4 Position : POSITION0;
    [[vk::location(1)]] float4 Color : COLOR0;
    [[vk::location(2)]] float2 UVcoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    [[vk::location(0)]] float4 Color : COLOR0;
    [[vk::location(1)]] float2 UVcoord : TEXCOORD0;
};

struct Camera
{
    float4x4 view;
    float4x4 proj;
    float3 position;
};

struct UBO
{
    double dt;
    float4x4 model;
    Camera cam;
};

// Descriptor loading in HLSL
ConstantBuffer <UBO> ubo : register(b0, space0);


VSOutput main(VSInput input, uint VertexIndex : SV_VertexID)
{
    VSOutput output = (VSOutput) 0;
    
    float4x4 camMatrix = mul(ubo.cam.proj, ubo.cam.view);
    output.Position = mul(camMatrix, mul(ubo.model, input.Position));
    
    output.Color = input.Color;
    output.UVcoord = input.UVcoord;
    
    return output;
}