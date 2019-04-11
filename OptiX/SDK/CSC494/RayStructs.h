#include <optix.h>
#include <optix_math.h>

using namespace optix;

#define FLT_MAX         1e30;

const int INTERSECTION_SAMPLES = 16;

static __device__ __inline__ uchar4 make_color(const float3& c)
{
	return make_uchar4(static_cast<unsigned char>(__saturatef(c.z)*255.99f),  /* B */
		static_cast<unsigned char>(__saturatef(c.y)*255.99f),  /* G */
		static_cast<unsigned char>(__saturatef(c.x)*255.99f),  /* R */
		255u);                                                 /* A */
}

struct IntersectionData
{
	uint rigidBodyId;

	float entryTval;
	float exitTval;

	float3 entryNormal;
	float3 exitNormal;
};

struct PerRayData_radiance
{
	float3 result;
	float3 missColor;
	float importance;
	int depth;

	bool hitObject;
	int numIntersections;
	IntersectionData intersections[INTERSECTION_SAMPLES];
	float closestTval; // The z-depth of the closest object in the scene
};

struct PerRayData_shadow
{
	float attenuation;
};
