#include <optix.h>
#include <optix_math.h>

using namespace optix;

#define FLT_MAX         1e30;

const int INTERSECTION_SAMPLES = 256;

static __device__ __inline__ uchar4 make_color(const float3& c)
{
	return make_uchar4(static_cast<unsigned char>(__saturatef(c.z)*255.99f),  /* B */
		static_cast<unsigned char>(__saturatef(c.y)*255.99f),  /* G */
		static_cast<unsigned char>(__saturatef(c.x)*255.99f),  /* R */
		255u);                                                 /* A */
}

struct PerRayData_radiance
{
	float3 result;
	float3 missColor;
	float  importance;
	int depth;

	int numIntersections;
	float2 intersections[INTERSECTION_SAMPLES]; // We'll store t-values of all intersections in this buffer
	float closestTval; // The z-depth of the closest object in the scene

	// Shading specific variables
	float3 closestShadingNormal;
};

