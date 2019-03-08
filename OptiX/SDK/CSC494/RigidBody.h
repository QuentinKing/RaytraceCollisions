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
	RigidBody(GeometryInstance mesh, float3 startingPosition, float mass, bool useGravity = true, float bounciness = 0.2) :
		mesh(mesh),
		position(startingPosition),
		mass(mass),
		useGravity(useGravity),
		bounciness(bounciness)
	{
		velocity = make_float3(0.0f, 0.0f, 0.0f);
		forceAccumulation = make_float3(0.0f, 0.0f, 0.0f);
		mesh["position"]->setFloat(startingPosition);
	};
	~RigidBody() {};

	void EulerStep(float deltaTime);
	void RegisterPlane(float3 point, float3 normal);
	void AddForce(float3 force);
	void UseGravity(bool useGravity);

	float GetMass();
	float3 GetPosition();
	float3 GetVelocity();
	float3 GetForces();

private:
	void ApplyGravity();
	void ApplyDrag();
	bool HandlePlaneCollisions(float3 positionDeriv);
	void CalculateDerivatives(float3 &positionDeriv, float3 &velocityDeriv, float deltaTime);

	GeometryInstance mesh;

	// TODO: Set based on geometry
	float kDrag = 0.1f; // Coefficient of drag
	bool useGravity;

	float mass;
	float bounciness;
	float3 position;
	float3 velocity;
	float3 forceAccumulation;

	std::vector<PlaneData> planeCollisions;
};
