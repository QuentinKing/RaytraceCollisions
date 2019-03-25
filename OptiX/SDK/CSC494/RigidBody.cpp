// OptiX
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include <sutil.h>

#include "RigidBody.h"

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
	Matrix3x3 rotation = QuaternionToRotation(quaternion);
	velocity = linearMomentum / mass;
	inertiaInv = rotation * inertiaBodyInv * rotation.transpose();
	spinVector = inertiaInv * angularMomentum;

	this->mesh->getGeometry()["velocity"]->setFloat(velocity);
	this->mesh->getGeometry()["spinVector"]->setFloat(spinVector);
}

/*
	Calculate velocity, inverse inertia, and spin vector
*/
void RigidBody::ODE(float deltaTime)
{
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
	Matrix3x3 rotation = QuaternionToRotation(quaternion);
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

Matrix3x3 RigidBody::QuaternionToRotation(float4 quaternion)
{
	normalize(quaternion);
	float temp[9];
	temp[0] = 1.0f - 2*quaternion.z*quaternion.z - 2*quaternion.w*quaternion.w;
	temp[1] = 2*quaternion.y*quaternion.z - 2*quaternion.x*quaternion.w;
	temp[2] = 2*quaternion.y*quaternion.w + 2*quaternion.x*quaternion.z;
	temp[3] = 2*quaternion.y*quaternion.z + 2*quaternion.x*quaternion.w;
	temp[4] = 1.0f - 2*quaternion.y*quaternion.y - 2*quaternion.w*quaternion.w;
	temp[5] = 2*quaternion.z*quaternion.w - 2*quaternion.x*quaternion.y;
	temp[6] = 2*quaternion.y*quaternion.w - 2*quaternion.x*quaternion.z;
	temp[7] = 2*quaternion.z*quaternion.w + 2*quaternion.x*quaternion.y;
	temp[8] = 1.0f - 2*quaternion.y*quaternion.y - 2*quaternion.z*quaternion.z;
	Matrix3x3 rotation(temp);
	return rotation;
}

float4 RigidBody::RotationToQuaternion(Matrix3x3 rotation)
{
	float4 quaternion = make_float4(0.0f);
	float trace;
	float s;
	trace = rotation[0] + rotation[4] + rotation[8];

	if (trace >= 0)
	{
		s = sqrt(trace + 1.0f);
		quaternion.x = 0.5f * s;
		s = 0.5 / s;
		quaternion.y = (rotation[7] - rotation[5]) * s;
		quaternion.z = (rotation[2] - rotation[6]) * s;
		quaternion.w = (rotation[3] - rotation[1]) * s;
	}
	else
	{
		int i = 0;

		if(rotation[4] > rotation[0])
			i = 1;

		if(rotation[8] > rotation[i*3+i])
			i = 2;

		switch (i)
		{
			case 0:
				s = sqrt((rotation[0] - (rotation[4] + rotation[8])) + 1.0f);
				quaternion.y = 0.5f * s;
				s = 0.5f / s;
				quaternion.z = (rotation[1] + rotation[3]) * s;
				quaternion.w = (rotation[6] + rotation[2]) * s;
				quaternion.x = (rotation[7] - rotation[5]) * s;
				break;
			case 1:
				s = sqrt((rotation[4] - (rotation[8] + rotation[0])) + 1.0f);
				quaternion.z = 0.5f * s;
				s = 0.5f / s;
				quaternion.w = (rotation[5] + rotation[7]) * s;
				quaternion.y = (rotation[1] + rotation[3]) * s;
				quaternion.x = (rotation[2] - rotation[6]) * s;
				break;
			case 2:
				s = sqrt((rotation[8] - (rotation[0] + rotation[4])) + 1.0f);
				quaternion.w = 0.5f * s;
				s = 0.5f / s;
				quaternion.y = (rotation[6] + rotation[2]) * s;
				quaternion.z = (rotation[5] + rotation[7]) * s;
				quaternion.x = (rotation[3] - rotation[1]) * s;
		}
	}
	return quaternion;
}
