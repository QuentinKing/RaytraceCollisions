// OptiX
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include <sutil.h>

#include "RigidBody.h"

using namespace optix;

// Update our rigid bodies data by computing a physics update
void RigidBody::EulerStep(float deltaTime)
{
	float3 positionDeriv;
	float3 velocityDeriv;

	ApplyGravity();
	ApplyDrag();

	CalculateDerivatives(positionDeriv, velocityDeriv, deltaTime);

	// If colliding with plane, handle collision and recalculate step
	if (HandlePlaneCollisions(positionDeriv))
	{
		CalculateDerivatives(positionDeriv, velocityDeriv, deltaTime);
	}

	// Update our rigidbody using the discrete derivative step
	position += positionDeriv;
	velocity += velocityDeriv;

	// Update our mesh's position
	mesh["position"]->setFloat(position);

	// Zero out force accumulation since we've passed it off into the velocity now
	forceAccumulation = make_float3(0.0f, 0.0f, 0.0f);
}

void RigidBody::CalculateDerivatives(float3 &positionDeriv, float3 &velocityDeriv, float deltaTime)
{
	positionDeriv = velocity;
	velocityDeriv = forceAccumulation / mass;

	positionDeriv *= deltaTime;
	velocityDeriv *= deltaTime;
}

bool RigidBody::HandlePlaneCollisions(float3 positionDeriv)
{
	float3 newPosition = position + positionDeriv;
	bool recalculate = false;
	for (auto i = planeCollisions.begin(); i != planeCollisions.end(); i++)
	{
		// Particle / plane collision detection
		// Collision if this is less than zero
		// Right now, hardcoding it as a sphere with radius 3
		float sdf = dot(newPosition - i->point, i->normal) - 3.0f;
		if (sdf < 0.0f)
		{
			recalculate = true;

			float3 vn = dot(i->normal, velocity) * i->normal;
			float3 vt = velocity - vn;

			velocity = vt - bounciness * vn;
		}
	}
	return recalculate;
}

void RigidBody::ApplyGravity()
{
	AddForce(make_float3(0.0f, mass * -9.80665, 0.0f));
}

void RigidBody::ApplyDrag()
{
	AddForce(-velocity * kDrag);
}

void RigidBody::RegisterPlane(float3 point, float3 normal)
{
	planeCollisions.push_back(PlaneData(point, normal));
}

void RigidBody::AddForce(float3 force)
{
	forceAccumulation += force;
}

float RigidBody::GetMass()
{
	return mass;
}

float3 RigidBody::GetPosition()
{
	return position;
}

float3 RigidBody::GetVelocity()
{
	return velocity;
}

float3 RigidBody::GetForces()
{
	return forceAccumulation;
}
