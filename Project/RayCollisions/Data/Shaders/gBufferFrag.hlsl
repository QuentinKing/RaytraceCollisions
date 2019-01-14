__import Shading;
__import DefaultVS;

struct GBuffer
{
	float4 wsPos    : SV_Target0;  // World Position
	float4 wsNorm   : SV_Target1;  // World Normal
	float4 matSpec  : SV_Target2;  // Material Specular
	float4 matDif   : SV_Target3;  // Material Diffuse 
};

GBuffer main(VertexOut vsOut, uint primID : SV_PrimitiveID, float4 pos : SV_Position)
{
	ShadingData hitPt = prepareShadingData(vsOut, gMaterial, gCamera.posW);

	GBuffer gBufOut;
	gBufOut.wsPos = float4(hitPt.posW, 1.f);
	gBufOut.wsNorm = float4(hitPt.N, length(hitPt.posW - gCamera.posW));
	gBufOut.matSpec = float4(hitPt.specular, hitPt.linearRoughness);
	gBufOut.matDif = float4(hitPt.diffuse, hitPt.opacity);

	return gBufOut;
}


