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

static __device__ __inline__ float3 schlick( float nDi, const float3& rgb )
{
  float r = fresnel_schlick(nDi, 5, rgb.x, 1);
  float g = fresnel_schlick(nDi, 5, rgb.y, 1);
  float b = fresnel_schlick(nDi, 5, rgb.z, 1);
  return make_float3(r, g, b);
}

struct IntersectionData
{
	uint rigidBodyId;
	float t;
	float3 normal;
};

struct PerRayData_radiance
{
	bool physicsRay;
	bool done;

	float3 origin;

	float3 result;
	float importance;
	int depth;

	int numIntersections;
	IntersectionData intersections[INTERSECTION_SAMPLES];
};

struct PerRayData_shadow
{
	float attenuation;
};
