#pragma once

#include <optixu/optixu_vector_types.h>

struct IntersectionResponse
{
#if defined(__cplusplus)
	typedef optix::float3 float3;
#endif
	float volume;			// Volume of intersection
	int entryId;			// Rigidbody id of the entry point of intersection
	int exitId;				// Rigidbody id of the exit point of intersection
	int collisionId;		// Id of the other rigidbody collided with
	float3 entryNormal;		// Surface normal of intersection entry
	float3 exitNormal;		// Surface normal of intersection exit
	float3 entryPoint;		// Entry point of intersection in world space
	float3 exitPoint;		// Exit point of intersection in world space
};

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
