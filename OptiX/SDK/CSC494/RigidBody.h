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
	RigidBody(Context context, GeometryInstance mesh, uint id, float3 startingPosition, float mass, bool useGravity = true, float bounciness = 0.2) :
		context(context),
		mesh(mesh),
		id(id),
		mass(mass),
		useGravity(useGravity),
		bounciness(bounciness)
	{
		// Create geometry group
		geometryGroup = context->createGeometryGroup();
		geometryGroup->setChildCount(1);
		geometryGroup->setChild(0, mesh);
		geometryGroup->setAcceleration(context->createAcceleration("NoAccel"));

		mesh->getGeometry()["id"]->setFloat(id);

		// Init state
		inertiaBody = make_matrix3x3(Matrix4x4::identity());
		inertiaBodyInv = inertiaBody.mat3inverse();

		position = startingPosition;
		quaternion = make_float4(1.0f, 0.0f, 0.0f, 0.0f);
		linearMomentum = make_float3(0.0f, 0.0f, 0.0f);
		angularMomentum = make_float3(0.0f, 0.0f, 0.0f);

		CalculateAuxiliaryVariables();

		force = make_float3(0.0f, 0.0f, 0.0f);
		torque = make_float3(0.0f, 0.0f, 0.0f);

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
	void AddForceAtRelativePosition(float3 force, float3 worldPosition);
	void AddForce(float3 force);
	void AddTorque(float3 torque);
	void UseGravity(bool useGravity);

	float3 GetVelocity();
	float3 GetSpin();

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

	// Rigidbody id
	uint id;

	// Rigidbody dynamics
	double mass;
	Matrix3x3 inertiaBody;
	Matrix3x3 inertiaBodyInv;

	// State space variable
	float3 position;
	float4 quaternion;
	float3 linearMomentum;
	float3 angularMomentum;

	// Derived members
	Matrix3x3 inertiaInv;
	float3 velocity;
	float3 spinVector;

	// Computed quantities
	float3 force;
	float3 torque;
};
