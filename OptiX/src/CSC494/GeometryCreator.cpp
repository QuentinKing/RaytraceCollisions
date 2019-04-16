// OptiX
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include <sutil.h>
#include <OptiXMesh.h>

#include "GeometryCreator.h"
#include "MaterialProperties.h"

using namespace optix;

GeometryInstance GeometryCreator::CreateSphere(float radius, MaterialProperties materialProps)
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
	Program sphere_ch = context->createProgramFromPTXString(scenePtx, materialProps.closestHitProgram);
	sphere_matl->setClosestHitProgram(0, sphere_ch);

	// Shadow caster program
	Program sphere_shadow = context->createProgramFromPTXString(scenePtx, "any_hit_shadow");
	sphere_matl->setAnyHitProgram(1, sphere_shadow);

	// Link material properties to the cuda files
	sphere_matl["ambientColorIntensity"]->setFloat(materialProps.ambientColor);
    sphere_matl["diffuseColorIntensity"]->setFloat(materialProps.diffuseColor);
	sphere_matl["specularColorIntensity"]->setFloat(materialProps.specularColor);
	sphere_matl["specularPower"]->setFloat(materialProps.specularPower);
	sphere_matl["fresnel"]->setFloat(materialProps.fresnel);
	sphere_matl["reflectivity"]->setFloat(materialProps.reflectivity);

	// Create Instance
	return context->createGeometryInstance(sphere, &sphere_matl, &sphere_matl + 1); // + 1 designates how many materials we are using
}

GeometryInstance GeometryCreator::CreateBox(float3 axisLengths, MaterialProperties materialProps)
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
	Program box_ch = context->createProgramFromPTXString(scenePtx, materialProps.closestHitProgram);
	box_matl->setClosestHitProgram(0, box_ch);

	// Shadow caster program
	Program box_shadow = context->createProgramFromPTXString(scenePtx, "any_hit_shadow");
	box_matl->setAnyHitProgram(1, box_shadow);

	// Link material properties to the cuda files
	box_matl["ambientColorIntensity"]->setFloat(materialProps.ambientColor);
    box_matl["diffuseColorIntensity"]->setFloat(materialProps.diffuseColor);
	box_matl["specularColorIntensity"]->setFloat(materialProps.specularColor);
	box_matl["specularPower"]->setFloat(materialProps.specularPower);
	box_matl["fresnel"]->setFloat(materialProps.fresnel);
	box_matl["reflectivity"]->setFloat(materialProps.reflectivity);

	return context->createGeometryInstance(box, &box_matl, &box_matl + 1);
}

GeometryInstance GeometryCreator::CreateMesh(std::string meshFilePath, MaterialProperties materialProps)
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
	Program mesh_ch = context->createProgramFromPTXString(scenePtx, materialProps.closestHitProgram);
	mesh_matl->setClosestHitProgram(0, mesh_ch);

	mesh_matl["ambientColorIntensity"]->setFloat(materialProps.ambientColor);
    mesh_matl["diffuseColorIntensity"]->setFloat(materialProps.diffuseColor);
	mesh_matl["specularColorIntensity"]->setFloat(materialProps.specularColor);
	mesh_matl["specularPower"]->setFloat(materialProps.specularPower);
	mesh_matl["fresnel"]->setFloat(materialProps.fresnel);
	mesh_matl["reflectivity"]->setFloat(materialProps.reflectivity);

    mesh.material = mesh_matl;

	// .obj are really small so just bump them up by default
	Matrix4x4 xform = Matrix4x4::identity();
	xform *= 0.5f;
	loadMesh(meshFilePath, mesh, xform);

	return mesh.geom_instance;
}
