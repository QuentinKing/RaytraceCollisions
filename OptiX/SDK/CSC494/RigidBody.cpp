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
	_velocity = _linearMomentum / _mass;
	_inertiaInv = _rotation * _inertiaBodyInv * _rotation.transpose();
	_spinVector = _inertiaInv * _angularMomentum;
}

/*
	Calculate velocity, inverse inertia, and spin vector
*/
void RigidBody::ODE(float deltaTime)
{
	// Add constant forces
	/*
	if (useGravity)
	{
		_force += make_float3(0.0f, _mass * -9.80665, 0.0f); // gravity
	}
	_force += -_velocity * kDrag; // drag
	*/

	// Calculate derivative of the state space
	float3 positionDot = _velocity * deltaTime;
	Matrix3x3 rotationDot = Star(_spinVector) * _rotation * deltaTime;
	float3 linearMomentumDot = _force * deltaTime;
	float3 angularMomentumDot = _torque * deltaTime;

	// Handle plane collisions
	/*
	if (HandlePlaneCollisions(positionDot))
	{
		positionDot = _velocity * deltaTime;
		rotationDot = Star(_spinVector) * _rotation * deltaTime;
		linearMomentumDot = _force * deltaTime;
		angularMomentumDot = _torque * deltaTime;
	}
	*/

	// Update rigidbody
	_position += positionDot;
	_rotation += rotationDot;
	_linearMomentum += linearMomentumDot;
	_angularMomentum += angularMomentumDot;

	// Pass off new trasnform to optix
	UpdateTransformNode();

	// Recalculate our derived variables
	CalculateAuxiliaryVariables();

	// Zero out force vectors
	_force = make_float3(0.0f, 0.0f, 0.0f);
	_torque = make_float3(0.0f, 0.0f, 0.0f);
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
	float temp[16] = {_rotation[0],_rotation[1],_rotation[2],_position.x,
					  _rotation[3],_rotation[4],_rotation[5],_position.y,
					  _rotation[6],_rotation[7],_rotation[8],_position.z,
					  0,0,0,1};
	Matrix4x4 newTransform(temp);
	transformNode->setMatrix(false, temp, NULL);
	MarkGroupAsDirty();
}

void RigidBody::AddForce(float3 force)
{
	_force += force;
}

void RigidBody::AddTorque(float3 torque)
{
	_torque += torque;
}

void RigidBody::SetRotation(float4 quaternion)
{
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




// TODO: Delete this. just testing out some physic responses
bool RigidBody::HandlePlaneCollisions(float3 positionDot)
{
	float3 newPosition = _position + positionDot;
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

			float3 vn = dot(i->normal, _velocity) * i->normal;
			float3 vt = _velocity - vn;

			_velocity = vt - bounciness * vn;
		}
	}
	return recalculate;
}
void RigidBody::RegisterPlane(float3 point, float3 normal)
{
	planeCollisions.push_back(PlaneData(point, normal));
}
