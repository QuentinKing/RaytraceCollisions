#pragma once

// OptiX
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include <sutil.h>

using namespace optix;


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

	GeometryInstance CreateSphere(float3 position, float radius);
	GeometryInstance CreatePlane(float3 anchor, float3 v1, float3 v2);
	GeometryInstance CreateBox(float3 boxMin, float3 boxMax);

private:
	Context context;
	const char* projectPrefix;
	const char* scenePtx;
};
