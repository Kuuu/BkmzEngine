cbuffer cbPerObject : register(b0)
{
    float4x4 worldViewProj;
};

struct VertexIn
{
    float3 posL : POSITION;
    float4 color : COLOR;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float4 color : COLOR;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    // Transform to homogeneous clip space.
    vout.posH = mul(float4(vin.posL, 1.0f), worldViewProj);
    
    //vout.posH = float4(vin.posL, 1.0f);
    // Just pass vertex color into the pixel shader.
    vout.color = vin.color;
    
    return vout;
}