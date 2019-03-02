#pragma once

// OptiX
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include <sutil.h>

using namespace optix;

class RigidBody
{
public:

	RigidBody(GeometryInstance mesh, float3 startingPosition, float mass) :
		mesh(mesh),
		position(startingPosition),
		mass(mass)
	{
		velocity = make_float3(0.0f, 0.0f, 0.0f);
		forceAccumulation = make_float3(0.0f, 0.0f, 0.0f);
		mesh["position"]->setFloat(startingPosition);
	};
	~RigidBody() {};

	void EulerStep(float deltaTime);
	void ApplyDrag();

	float GetMass()
	{
		return mass;
	}

	float3 GetPosition()
	{
		return position;
	}

	float3 GetVelocity()
	{
		return velocity;
	}

	void AddForce(float3 force)
	{
		forceAccumulation += force;
	}

	float3 GetForces()
	{
		return forceAccumulation;
	}

private:
	GeometryInstance mesh;

	// TODO: Set based on geometry
	float kDrag = 0.1f; // Coefficient of drag

	float mass;
	float3 position;
	float3 velocity;
	float3 forceAccumulation;
};
