struct VertexOuput
{
	float4 position : SV_POSITION;
	float4 color    : COLOR;
};

VertexOuput vertex_shader(float4 position : position, float4 color : rgba)
{
	VertexOuput output;

	output.position = position;
	output.color    = color;

	return output;
}


float4 pixel_shader(float4 position : SV_POSITION, float4 color : COLOR) : SV_TARGET
{
	return color;
}
