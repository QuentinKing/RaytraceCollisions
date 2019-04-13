// OptiX
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include <sutil.h>

#include "RigidBody.h"
#include "MathHelpers.h"

using namespace optix;

// Update our rigid bodies data by computing a physics update
void RigidBody::EulerStep(float deltaTime)
{
	ODE(deltaTime);
}

/*
	Calculate velocity, inverse inertia, and spin vector
*/
void RigidBody::CalculateAuxiliaryVariables()
{
	Matrix3x3 rotation = MathHelpers::QuaternionToRotation(quaternion);
	velocity = linearMomentum / mass;
	inertiaInv = rotation * inertiaBodyInv * rotation.transpose();
	spinVector = inertiaInv * angularMomentum;

	this->geometryInstance->getGeometry()["velocity"]->setFloat(velocity);
	this->geometryInstance->getGeometry()["spinVector"]->setFloat(spinVector);
}

/*
	Calculate velocity, inverse inertia, and spin vector
*/
void RigidBody::ODE(float deltaTime)
{
	if (isStatic)
	{
		return;
	}

	// Apply Gravity
	if (useGravity)
	{
		force += make_float3(0.0f, mass * -9.80665, 0.0f);
	}

	// Apply drag
	force += -linearMomentum * kDrag;
	torque += -angularMomentum * kDrag;

	// Calculate derivative of the state space
	float3 positionDot = velocity * deltaTime;
	float3 linearMomentumDot = force * deltaTime;
	float3 angularMomentumDot = torque * deltaTime;

	float3 quaternionV = make_float3(quaternion.y, quaternion.z, quaternion.w);
	float quaternionDotS = -dot(quaternionV, spinVector);
	float3 quaternionDotV = quaternion.x * spinVector + cross(spinVector, quaternionV);
	float4 quaternionDot = 0.5 * make_float4(quaternionDotS, quaternionDotV.x, quaternionDotV.y, quaternionDotV.z);

	// Update rigidbody
	position += positionDot;
	quaternion += quaternionDot;
	quaternion = normalize(quaternion);
	linearMomentum += linearMomentumDot;
	angularMomentum += angularMomentumDot;

	// Pass off new trasnform to optix
	UpdateTransformNode();

	// Recalculate our derived variables
	CalculateAuxiliaryVariables();

	// Zero out force vectors
	force = make_float3(0.0f, 0.0f, 0.0f);
	torque = make_float3(0.0f, 0.0f, 0.0f);
}

/*
	Calculates the star matrix (cross product) of the given vector
*/
Matrix3x3 RigidBody::Star(float3 vector)
{
	float temp[9] = { 0.0f , -vector.z , vector.y, vector.z, 0.0f, -vector.x, -vector.y, vector.x, 0.0f };
	return Matrix3x3(temp);
}

/*
	Update transformation matrix that optix uses
*/
void RigidBody::UpdateTransformNode()
{
	Matrix3x3 rotation = MathHelpers::QuaternionToRotation(quaternion);
	float temp[16] = {rotation[0],rotation[1],rotation[2],position.x,
					  rotation[3],rotation[4],rotation[5],position.y,
					  rotation[6],rotation[7],rotation[8],position.z,
					  0,0,0,1};
	Matrix4x4 newTransform(temp);
	transformNode->setMatrix(false, temp, NULL);
	MarkGroupAsDirty();
}

/*
	Adds a force at a position relative to the center of mass.
	The position given should be close to the surface of the object
*/
void RigidBody::AddForceAtPosition(float3 force, float3 worldPosition)
{
	this->force += force;
	this->torque += cross(worldPosition-this->position, force) * 0.01;
}

/*
	Adds an impulse at the given position.
	The position given should be close to the surface of the object
*/
void RigidBody::AddImpulseAtPosition(float3 impulse, float3 worldPosition)
{
	this->linearMomentum += impulse;
	this->angularMomentum += cross(worldPosition-this->position, impulse) * 0.01;
}

void RigidBody::AddForce(float3 force)
{
	this->force += force;
}

void RigidBody::AddTorque(float3 torque)
{
	this->torque += torque;
}

void RigidBody::UseGravity(bool useGravity)
{
	this->useGravity = useGravity;
}

float3 RigidBody::GetVelocity()
{
	return velocity;
}

float3 RigidBody::GetSpin()
{
	return spinVector;
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
