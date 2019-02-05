#pragma once

#include <optixu/optixu_vector_types.h>

struct BasicLight
{
#if defined(__cplusplus)
	typedef optix::float3 float3;
#endif
	float3 pos;
	float3 color;
	int    casts_shadow;
	int    padding;      // make this structure 32 bytes -- powers of two are your friend!
};
