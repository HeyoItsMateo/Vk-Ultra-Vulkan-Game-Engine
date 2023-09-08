struct PSInput
{
    float4 Position : SV_POSITION;
    [[vk::location(0)]] float4 Normal : NORMAL;
    [[vk::location(1)]] float4 Color : COLOR;
    [[vk::location(2)]] float2 UVcoord : TEXCOORD;
};

struct PSOutput
{
    [[vk::location(0)]] float4 Color : SV_TARGET;
};

PSOutput main(PSInput input)
{
    PSOutput output;
    //output.Color = tex2D.Sample(sample2D, input.UVcoord); //TODO: figure out the right syntax for this
    output.Color = input.Color;
    return output;
}