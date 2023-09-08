struct PSInput
{
    float4 Position : SV_POSITION;
    [[vk::location(0)]] float4 Color : COLOR0;
    [[vk::location(1)]] float4 Normal : NORMAL0;
    [[vk::location(2)]] float2 UVcoord : TEXCOORD0;
};

// Descriptor loading in HLSL
Texture2D tex2D : register(t0, space1); // TODO: Figure out texture loading and sampling in HLSL
sampler sample2D : register(s0, space1); // texture sampler at (binding = 0, set = 1)

struct PSOutput
{
    [[vk::location(0)]] float4 Color : SV_TARGET;
};

PSOutput main(PSInput input)
{
    PSOutput output = (PSOutput) 0;
    //output.Color = tex2D.Sample(sample2D, input.UVcoord); //TODO: figure out the right syntax for this
    return input.Color;
}