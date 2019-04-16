#pragma once

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

using namespace optix;

/*
	Wrapper for phong shading parameters / closest hit program
*/
class MaterialProperties
{
public:
	MaterialProperties(const char* closestHitProgram, float3 ambient, float3 diffuse, float3 specular, float specularPower, float3 fresnel, float3 reflectivity) :
		closestHitProgram(closestHitProgram), ambientColor(ambient), diffuseColor(diffuse), specularColor(specular), specularPower(specularPower),
		fresnel(fresnel), reflectivity(reflectivity)
	{

	};
	~MaterialProperties() {};

	const char* closestHitProgram;
	float3 ambientColor;
	float3 diffuseColor;
	float3 specularColor;
	float specularPower;
	float3 fresnel;
	float3 reflectivity;
};
