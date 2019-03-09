#pragma once

// OptiX
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include <vector>

#include <sutil.h>

using namespace optix;

struct PlaneData
{
	float3 point;
	float3 normal;

	PlaneData(float3 point, float3 normal) :
		point(point),
		normal(normal)
	{
	}
};

class RigidBody
{
public:
	RigidBody(Context context, GeometryInstance mesh, float3 startingPosition, float mass, bool useGravity = true, float bounciness = 0.2) :
		context(context),
		mesh(mesh),
		mass(mass),
		useGravity(useGravity),
		bounciness(bounciness)
	{
		// Create geometry group
		geometryGroup = context->createGeometryGroup();
		geometryGroup->setChildCount(1);
		geometryGroup->setChild(0, mesh);
		geometryGroup->setAcceleration(context->createAcceleration("NoAccel"));

		// Create transformation node
		transformNode = context->createTransform();
		transformNode->setChild(geometryGroup);
		rotation = make_float4(0, 0, 0, 1);
		float identity[16] = { 1,0,0,startingPosition.x,
							   0,1,0,startingPosition.y,
							   0,0,1,startingPosition.z,
							   0,0,0,1 };
		std::copy(identity, identity + 16, transformMatrix);
		transformNode->setMatrix(false, transformMatrix, NULL);
		MarkGroupAsDirty();

		velocity = make_float3(0.0f, 0.0f, 0.0f);
		forceAccumulation = make_float3(0.0f, 0.0f, 0.0f);
	};
	~RigidBody() {};

	void EulerStep(float deltaTime);
	void RegisterPlane(float3 point, float3 normal);
	void AddForce(float3 force);
	void SetPosition(float3 position);
	void AddPositionRelative(float3 vector);
	void SetRotation(float4 quaternion);
	void UseGravity(bool useGravity);

	GeometryGroup GetGeometryGroup();
	Transform GetTransform();

	float GetMass();
	float3 GetPosition();
	float3 GetVelocity();
	float3 GetForces();

private:
	void ApplyGravity();
	void ApplyDrag();
	void MarkGroupAsDirty();
	bool HandlePlaneCollisions(float3 positionDeriv);
	void CalculateDerivatives(float3 &positionDeriv, float3 &velocityDeriv, float deltaTime);

	Context context;
	Transform transformNode;
	GeometryGroup geometryGroup;
	GeometryInstance mesh;

	float transformMatrix[16];

	// TODO: Set based on geometry
	float kDrag = 0.1f; // Coefficient of drag
	bool useGravity;

	float mass;
	float bounciness;
	float4 rotation; // As a quaternion
	float3 velocity;
	float3 forceAccumulation;

	std::vector<PlaneData> planeCollisions;
};
