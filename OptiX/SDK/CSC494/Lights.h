#pragma once

#include <optixu/optixu_vector_types.h>


struct RigidbodyMotion
{
#if defined(__cplusplus)
	typedef optix::float3 float3;
#endif
	float3 velocity;
	float3 spin;
};

struct Light
{
#if defined(__cplusplus)
  typedef optix::float3 float3;
#endif
  float3 pos;
  float3 color;
  int    casts_shadow;
  int    padding; // Makes struct 32 bytes
};
