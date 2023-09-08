struct VSInput
{
    [[vk::location(0)]] float4 Position : POSITION;
    [[vk::location(1)]] float4 Normal : NORMAL;
    [[vk::location(2)]] float4 Color : COLOR;
    [[vk::location(3)]] float2 UVcoord : TEXCOORD;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    [[vk::location(0)]] float4 Normal : NORMAL;
    [[vk::location(1)]] float4 Color : COLOR;
    [[vk::location(2)]] float2 UVcoord : TEXCOORD;
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
ConstantBuffer<UBO> ubo : register(b0, space0);

VSOutput main(VSInput input, uint VertexIndex : SV_VertexID)
{
    float4x4 camMatrix = ubo.cam.proj * ubo.cam.view;
    
    VSOutput output;
    output.Position = mul(camMatrix, input.Position);
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.UVcoord = input.UVcoord;
    
    return output;
}