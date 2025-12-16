struct VertexOut
{
    float4 posH : SV_POSITION;
    float4 color : COLOR;
};

float4 PS(VertexOut pin) : SV_TARGET
{
	return pin.color;
}