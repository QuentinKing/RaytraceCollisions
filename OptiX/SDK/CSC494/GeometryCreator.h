#pragma once

// OptiX
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include <sutil.h>

using namespace optix;

#include "MaterialProperties.h"

class GeometryCreator
{
public:

	GeometryCreator(Context context, const char* projectPrefix, const char* sceneName) :
		context(context),
		projectPrefix(projectPrefix)
	{
		scenePtx = sutil::getPtxString(projectPrefix, sceneName);
	};
	~GeometryCreator() {};

	GeometryInstance CreateSphere(float radius, MaterialProperties materialProps);
	GeometryInstance CreateBox(float3 axisLengths, MaterialProperties materialProps);
	GeometryInstance CreateMesh(std::string meshFilePath, MaterialProperties materialProps);

private:
	Context context;
	const char* projectPrefix;
	const char* scenePtx;
};
