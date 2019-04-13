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

	GeometryInstance CreateSphere(float radius, const char* materialProgram);
	GeometryInstance CreateBox(float3 axisLengths, const char* materialProgram);
	GeometryInstance CreateMesh(std::string meshFilePath, const char* materialProgram);

private:
	Context context;
	const char* projectPrefix;
	const char* scenePtx;
};
