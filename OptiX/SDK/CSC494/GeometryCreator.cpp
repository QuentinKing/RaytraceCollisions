// OptiX
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include <sutil.h>

#include "GeomteryCreator.h"

using namespace optix;

GeometryInstance GeometryCreator::CreateSphere(float3 position, float radius)
{
	// Create geometry and transform
	Geometry sphere = context->createGeometry();
	sphere->setPrimitiveCount(1u);
	float4 sphereData = make_float4(position, radius);

	// Create programs
	const char* ptx = sutil::getPtxString(projectPrefix, "sphere_model.cu");
	Program sphere_bounds = context->createProgramFromPTXString(ptx, "bounds");
	Program sphere_intersect = context->createProgramFromPTXString(ptx, "robust_intersect");
	sphere->setBoundingBoxProgram(sphere_bounds);
	sphere->setIntersectionProgram(sphere_intersect);
	sphere["sphere"]->setFloat(sphereData);

	// Create material
	Material sphere_matl = context->createMaterial();
	Program sphere_ch = context->createProgramFromPTXString(scenePtx, "closest_hit_radiance0");
	Program sphere_ah = context->createProgramFromPTXString(scenePtx, "any_hit");
	sphere_matl->setClosestHitProgram(0, sphere_ch);
	sphere_matl->setAnyHitProgram(0, sphere_ah);

	// Create Instance
	return context->createGeometryInstance(sphere, &sphere_matl, &sphere_matl + 1); // + 1 designates how many materials we are using
}

GeometryInstance GeometryCreator::CreatePlane(float3 anchor, float3 v1, float3 v2)
{
	// Create geometry
	Geometry parallelogram = context->createGeometry();
	parallelogram->setPrimitiveCount(1u);

	// Create programs
	const char* ptx = sutil::getPtxString(projectPrefix, "parallelogram.cu");
	parallelogram->setBoundingBoxProgram(context->createProgramFromPTXString(ptx, "bounds"));
	parallelogram->setIntersectionProgram(context->createProgramFromPTXString(ptx, "intersect"));

	// Calculate geometry
	float3 normal = cross(v2, v1);
	normal = normalize(normal);
	float d = dot(normal, anchor);
	v1 *= 1.0f / dot(v1, v1);
	v2 *= 1.0f / dot(v2, v2);
	float4 plane = make_float4(normal, d);
	parallelogram["plane"]->setFloat(plane);
	parallelogram["v1"]->setFloat(v1);
	parallelogram["v2"]->setFloat(v2);
	parallelogram["anchor"]->setFloat(anchor);

	Material plane_matl = context->createMaterial();
	Program floor_ch = context->createProgramFromPTXString(scenePtx, "closest_hit_radiance0");
	plane_matl->setClosestHitProgram(0, floor_ch);

	return context->createGeometryInstance(parallelogram, &plane_matl, &plane_matl + 1);
}

GeometryInstance GeometryCreator::CreateBox(float3 boxMin, float3 boxMax)
{
	// Create geometry
	Geometry box = context->createGeometry();
	box->setPrimitiveCount(1u);

	// Create programs
	const char *ptx = sutil::getPtxString(projectPrefix, "box.cu");
	Program box_bounds = context->createProgramFromPTXString(ptx, "box_bounds");
	Program box_intersect = context->createProgramFromPTXString(ptx, "box_intersect");

	box->setBoundingBoxProgram(box_bounds);
	box->setIntersectionProgram(box_intersect);
	box["boxmin"]->setFloat(-2.0f, 0.0f, -2.0f);
	box["boxmax"]->setFloat(2.0f, 7.0f, 2.0f);

	// Create Material
	Material box_matl = context->createMaterial();
	Program box_ch = context->createProgramFromPTXString(scenePtx, "closest_hit_radiance0");
	box_matl->setClosestHitProgram(0, box_ch);

	return context->createGeometryInstance(box, &box_matl, &box_matl + 1);
}
