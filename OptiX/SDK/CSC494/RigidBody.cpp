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

	// Apply continous forces
	if (this->useGravity)
		ApplyGravity();
	ApplyDrag();

	CalculateDerivatives(positionDeriv, velocityDeriv, deltaTime);

	// If colliding with plane, handle collision and recalculate step
	if (HandlePlaneCollisions(positionDeriv))
	{
		CalculateDerivatives(positionDeriv, velocityDeriv, deltaTime);
	}

	// Update our rigidbody using the discrete derivative step
	AddPositionRelative(positionDeriv);
	velocity += velocityDeriv;

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
	float3 newPosition = GetPosition() + positionDeriv;
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

// TODO: Delete this. just testing out some physic responses
void RigidBody::RegisterPlane(float3 point, float3 normal)
{
	planeCollisions.push_back(PlaneData(point, normal));
}

void RigidBody::AddForce(float3 force)
{
	forceAccumulation += force;
}

void RigidBody::SetPosition(float3 position)
{
	transformMatrix[3] = position.x;
	transformMatrix[7] = position.y;
	transformMatrix[11] = position.z;
	transformNode->setMatrix(false, transformMatrix, NULL);
	MarkGroupAsDirty();
}

void RigidBody::AddPositionRelative(float3 vector)
{
	transformMatrix[3] += vector.x;
	transformMatrix[7] += vector.y;
	transformMatrix[11] += vector.z;
	transformNode->setMatrix(false, transformMatrix, NULL);
	MarkGroupAsDirty();
}

void RigidBody::SetRotation(float4 quaternion)
{
	rotation = quaternion;
	transformMatrix[0] = 1.0f - 2*quaternion.y*quaternion.y - 2*quaternion.z*quaternion.z;
	transformMatrix[1] = 2*quaternion.x*quaternion.y - 2*quaternion.z*quaternion.w;
	transformMatrix[2] = 2*quaternion.x*quaternion.z + 2*quaternion.y*quaternion.w;
	transformMatrix[4] = 2*quaternion.x*quaternion.y + 2*quaternion.z*quaternion.w;
	transformMatrix[5] = 1.0f - 2*quaternion.x*quaternion.x - 2*quaternion.z*quaternion.z;
	transformMatrix[6] = 2*quaternion.y*quaternion.z - 2*quaternion.x*quaternion.w;
	transformMatrix[8] = 2*quaternion.x*quaternion.z - 2*quaternion.y*quaternion.w;
	transformMatrix[9] = 2*quaternion.y*quaternion.z + 2*quaternion.x*quaternion.w;
	transformMatrix[10] = 1.0f - 2*quaternion.x*quaternion.x - 2*quaternion.y*quaternion.y;
	transformNode->setMatrix(false, transformMatrix, NULL);
	MarkGroupAsDirty();
}

void RigidBody::UseGravity(bool useGravity)
{
	this->useGravity = useGravity;
}

float RigidBody::GetMass()
{
	return mass;
}

float3 RigidBody::GetPosition()
{
	return make_float3(transformMatrix[3], transformMatrix[7], transformMatrix[11]);
}

float3 RigidBody::GetVelocity()
{
	return velocity;
}

float3 RigidBody::GetForces()
{
	return forceAccumulation;
}

GeometryGroup RigidBody::GetGeometryGroup()
{
	return geometryGroup;
}

Transform RigidBody::GetTransform()
{
	return transformNode;
}

void RigidBody::MarkGroupAsDirty()
{
	geometryGroup->getAcceleration()->markDirty();
}
