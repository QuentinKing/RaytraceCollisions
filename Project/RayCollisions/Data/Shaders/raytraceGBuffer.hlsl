// Required for Falcor
#include "HostDeviceSharedMacros.h"
#include "HostDeviceData.h" 

// Import Falcor stuff
__import Raytracing;
__import ShaderCommon;
__import Shading; 
__import Lights;

// Need to define a payload
struct SimpleRayPayload
{
	bool dummyValue;
};


[shader("raygeneration")]
void GBufferRayGen()
{
	// Get ray direction (world space)
	float2 pixelCenter = (DispatchRaysIndex().xy + float2(0.5f, 0.5f)) / DispatchRaysDimensions().xy;
	float2 ndc = float2(2, -2) * pixelCenter + float2(-1, 1);
	float3 rayDir = ndc.x * gCamera.cameraU + ndc.y * gCamera.cameraV + gCamera.cameraW;

	// Build ray structure
	RayDesc ray;
	ray.Origin = gCamera.posW;			// Start ray at camera's position in world space
	ray.Direction = normalize(rayDir);  // Normalized ray direction
	ray.TMin = 0.0f;					// Minimum distance for hit to be reported
	ray.TMax = 1e+38f;					// Maximum distance for hit to be reported

	// Init payload
	SimpleRayPayload rayPayload = { false };

	// Trace our ray
	TraceRay(gRtScene,                        // A Falcor built-in containing the raytracing acceleration structure
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,  // Flags
		0xFF,                                 // Instance inclusion mask.  0xFF => no instances discarded from this mask
		0,                                    // Hit group to index (i.e., when intersecting, call hit shader #0)
		hitProgramCount,                      // Number of hit groups ('hitProgramCount' is built-in from Falcor with the right number)
		0,                                    // Miss program index (i.e., when missing, call miss shader #0)
		ray,                                  // Data structure describing the ray to trace
		rayPayload);                          // Our user-defined ray payload structure to store intermediate results
}


// Output textures, bindings constructed on CPU side
RWTexture2D<float4> gWsPos;
RWTexture2D<float4> gWsNorm;
RWTexture2D<float4> gMatDif;
RWTexture2D<float4> gMatSpec;
RWTexture2D<float4> gMatEmissive;

cbuffer MissShaderCB
{
	float3  gBgColor;
};

// Miss shader
[shader("miss")]
void PrimaryMiss(inout SimpleRayPayload)
{
	gMatDif[DispatchRaysIndex().xy] = float4(gBgColor, 1.0f);
}

// Any hit shader
[shader("anyhit")]
void PrimaryAnyHit(inout SimpleRayPayload, BuiltInTriangleIntersectionAttributes attribs)
{
	// Think we mostly care about this for trasparency, for now just ignore this
	IgnoreHit();
}

// Closest hit shader
[shader("closesthit")]
void PrimaryClosestHit(inout SimpleRayPayload, BuiltInTriangleIntersectionAttributes attribs)
{
	// Get pixel index
	uint2  launchIndex = DispatchRaysIndex().xy;

	VertexOut  vsOut = getVertexAttributes(PrimitiveIndex(), attribs);
	ShadingData shadeData = prepareShadingData(vsOut, gMaterial, gCamera.posW, 0);

	float3 phong = float3(0.0, 0.0, 0.0);

	// Iterate over the lights for phong shading
	// TODO: Probably best not to do shading for each hit, move to it's own pass later
	for (int index = 0; index < gLightsCount; index++)
	{
		float distToLight;      // Distance to light (Which we don't care about since I'm using it as a directional light)
		float3 lightIntensity;  // Color of the light
		float3 toLight;         // Direction of light from current pixel

		// "Lights.slang" contains definition for this data structure
		LightSample ls;
		if (gLights[index].type == LightDirectional)
		{
			ls = evalDirectionalLight(gLights[index], shadeData.posW);
		}
		else
		{
			ls = evalPointLight(gLights[index], shadeData.posW);
		}

		toLight = normalize(ls.L);
		lightIntensity = ls.diffuse;
		distToLight = length(ls.posW - shadeData.posW);

		phong += shadeData.diffuse * lightIntensity * dot(toLight, shadeData.N);
	}

	gWsPos[launchIndex] = float4(shadeData.posW, 1.f);
	gWsNorm[launchIndex] = float4(shadeData.N, length(shadeData.posW - gCamera.posW));
	gMatDif[launchIndex] = float4(phong, shadeData.opacity);
	gMatSpec[launchIndex] = float4(shadeData.specular, shadeData.linearRoughness);
	gMatEmissive[launchIndex] = float4(shadeData.emissive, 0.f);
}
