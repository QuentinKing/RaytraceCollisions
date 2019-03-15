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
		useGravity(useGravity),
		bounciness(bounciness)
	{
		// Create geometry group
		geometryGroup = context->createGeometryGroup();
		geometryGroup->setChildCount(1);
		geometryGroup->setChild(0, mesh);
		geometryGroup->setAcceleration(context->createAcceleration("NoAccel"));

		// Init state
		_mass = mass;
		_inertiaBody = make_matrix3x3(Matrix4x4::identity());
		_inertiaBodyInv = _inertiaBody.mat3inverse();

		_position = startingPosition;
		_quaternion = make_float4(1.0f, 0.0f, 0.0f, 0.0f);
		_linearMomentum = make_float3(0.0f, 0.0f, 0.0f);
		_angularMomentum = make_float3(0.0f, 0.0f, 0.0f);

		CalculateAuxiliaryVariables();

		_force = make_float3(0.0f, 0.0f, 0.0f);
		_torque = make_float3(0.0f, 0.0f, 0.0f);

		// Create transformation node
		transformNode = context->createTransform();
		transformNode->setChild(geometryGroup);
		float identity[16] = { 1,0,0,startingPosition.x,
							   0,1,0,startingPosition.y,
							   0,0,1,startingPosition.z,
							   0,0,0,1 };
		std::copy(identity, identity + 16, transformMatrix);
		transformNode->setMatrix(false, transformMatrix, NULL);

		MarkGroupAsDirty();
	};
	~RigidBody() {};

	void EulerStep(float deltaTime);
	void RegisterPlane(float3 point, float3 normal);
	void AddForce(float3 force);
	void AddTorque(float3 torque);
	void UseGravity(bool useGravity);

	GeometryGroup GetGeometryGroup();
	Transform GetTransform();

private:
	void MarkGroupAsDirty();
	void CalculateAuxiliaryVariables();
	void ODE(float deltaTime);
	Matrix3x3 Star(float3 vector);
	void UpdateTransformNode();

	Matrix3x3 QuaternionToRotation(float4 quaternion);
	float4 RotationToQuaternion(Matrix3x3 rotation);

	// Optix variable
	Context context;
	Transform transformNode;
	GeometryGroup geometryGroup;
	GeometryInstance mesh;

	float transformMatrix[16];

	// TODO: Set based on geometry
	float kDrag = 0.1f; // Coefficient of drag
	bool useGravity;

	float bounciness;

	// Rigidbody dynamics
	double _mass;
	Matrix3x3 _inertiaBody;
	Matrix3x3 _inertiaBodyInv;

	// State space variable
	float3 _position;
	float4 _quaternion;
	float3 _linearMomentum;
	float3 _angularMomentum;

	// Derived members
	Matrix3x3 _inertiaInv;
	float3 _velocity;
	float3 _spinVector;

	// Computed quantities
	float3 _force;
	float3 _torque;

	// Testing
	bool HandlePlaneCollisions(float3 positionDeriv);
	std::vector<PlaneData> planeCollisions;
};
