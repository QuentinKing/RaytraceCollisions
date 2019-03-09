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

	GeometryInstance CreateSphere(float radius);
	GeometryInstance CreateBox(float3 axisLengths);

	GeometryInstance CreatePlane(float3 anchor, float3 v1, float3 v2);


private:
	Context context;
	const char* projectPrefix;
	const char* scenePtx;
};
