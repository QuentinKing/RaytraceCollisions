// OptiX
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include <sutil.h>
#include <OptiXMesh.h>

#include "GeometryCreator.h"

using namespace optix;

GeometryInstance GeometryCreator::CreateSphere(float radius, const char* materialProgram)
{
	// Create geometry and transform
	Geometry sphere = context->createGeometry();
	sphere->setPrimitiveCount(1u);

	// Create programs
	const char* ptx = sutil::getPtxString(projectPrefix, "sphere_model.cu");
	Program sphere_bounds = context->createProgramFromPTXString(ptx, "bounds");
	Program sphere_intersect = context->createProgramFromPTXString(ptx, "robust_intersect");
	sphere->setBoundingBoxProgram(sphere_bounds);
	sphere->setIntersectionProgram(sphere_intersect);
	sphere["radius"]->setFloat(radius);

	// Create material
	Material sphere_matl = context->createMaterial();
	//Program sphere_ch = context->createProgramFromPTXString(scenePtx, "closest_hit_radiance_sphere");
	Program sphere_ch = context->createProgramFromPTXString(scenePtx, materialProgram);
	sphere_matl->setClosestHitProgram(0, sphere_ch);

	// Shadow caster program
	Program sphere_shadow = context->createProgramFromPTXString(scenePtx, "any_hit_shadow");
	sphere_matl->setAnyHitProgram(1, sphere_shadow);

	// Hardcode in some material properties for now (color, etc..)
	sphere_matl["ambientColorIntensity"]->setFloat( 0.3f, 0.3f, 0.3f );
    sphere_matl["diffuseColorIntensity"]->setFloat( 0.6f, 0.7f, 0.8f );
	sphere_matl["specularColorIntensity"]->setFloat( 0.8f, 0.9f, 0.8f );
	sphere_matl["specularPower"]->setFloat( 88.0f );

	// Create Instance
	return context->createGeometryInstance(sphere, &sphere_matl, &sphere_matl + 1); // + 1 designates how many materials we are using
}

GeometryInstance GeometryCreator::CreateBox(float3 axisLengths, const char* materialProgram)
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
	box["axisLengths"]->setFloat(axisLengths);

	// Create Material
	Material box_matl = context->createMaterial();
	//Program box_ch = context->createProgramFromPTXString(scenePtx, "closest_hit_radiance_box");
	Program box_ch = context->createProgramFromPTXString(scenePtx, materialProgram);
	box_matl->setClosestHitProgram(0, box_ch);

	// Shadow caster program
	Program box_shadow = context->createProgramFromPTXString(scenePtx, "any_hit_shadow");
	box_matl->setAnyHitProgram(1, box_shadow);

	// Hardcode in some material properties for now (color, etc..)
	box_matl["ambientColorIntensity"]->setFloat( 0.1f, 0.1f, 0.1f );
    box_matl["diffuseColorIntensity"]->setFloat( 0.8f, 0.2f, 0.8f );
	box_matl["specularColorIntensity"]->setFloat( 0.8f, 0.9f, 0.8f );
	box_matl["specularPower"]->setFloat( 88.0f );

	return context->createGeometryInstance(box, &box_matl, &box_matl + 1);
}

GeometryInstance GeometryCreator::CreateMesh(std::string meshFilePath, const char* materialProgram)
{
	const char* ptx = sutil::getPtxString(projectPrefix, "triangle_mesh.cu");

	// Override default programs with our own
	Program intersection = context->createProgramFromPTXString(ptx, "mesh_intersect_refine");
	Program bounds = context->createProgramFromPTXString(ptx, "mesh_bounds");

	OptiXMesh mesh;
    mesh.context = context;
    mesh.intersection = intersection;
    mesh.bounds = bounds;

	Material mesh_matl = context->createMaterial();
	Program mesh_ch = context->createProgramFromPTXString(scenePtx, materialProgram);
	mesh_matl->setClosestHitProgram(0, mesh_ch);

	mesh_matl["ambientColorIntensity"]->setFloat( 0.8f, 0.3f, 0.2f );
    mesh_matl["diffuseColorIntensity"]->setFloat( 0.3f, 0.2f, 0.8f );
	mesh_matl["specularColorIntensity"]->setFloat( 0.3f, 0.8f, 0.2f );
	mesh_matl["specularPower"]->setFloat( 8.0f );

    mesh.material = mesh_matl;

	Matrix4x4 xform = Matrix4x4::identity();
	xform *= 40.0f;
	loadMesh(meshFilePath, mesh, xform);

	return mesh.geom_instance;
}
