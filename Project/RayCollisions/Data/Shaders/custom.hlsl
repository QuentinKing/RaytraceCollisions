// Shader parameters passed from CPU
cbuffer PerFrameCB
{
    uint gFrameCount;
	float gMultValue;
	float3 gColor;
}

// Fragment pixel shader
float4 main(float2 texC : TEXCOORD, float4 pos : SV_Position) : SV_Target0
{
	// Compute a nice looking shader
	float sinusoid = 0.5 * (1.0f + sin(0.001f * gMultValue * (dot(pos.xy, pos.xy) + gFrameCount / gMultValue) ));

	// Return to framebuffer
    return float4(sinusoid, sinusoid, sinusoid, 1.0f) * float4(gColor.rgb, 1.0f);
}
