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
	RigidBody(Context context, const char* projectPrefix, const char* sceneName, GeometryInstance geometryInstance,
			  uint id, float3 startingPosition, float mass, const char* acceleration, bool isStatic,
			  bool useGravity = true, float drag = 0.5f) :
		context(context),
		geometryInstance(geometryInstance),
		id(id),
		mass(mass),
		isStatic(isStatic),
		useGravity(useGravity),
		kDrag(drag)
	{
		// Create geometry group
		geometryGroup = context->createGeometryGroup();
		geometryGroup->setChildCount(1);
		geometryGroup->setChild(0, geometryInstance);
		geometryGroup->setAcceleration(context->createAcceleration(acceleration));

		geometryInstance->getGeometry()["id"]->setFloat(id);

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

		// Link any-hit program based on if the object is static or not
		Program anyHit;
		const char* scenePtx = sutil::getPtxString(projectPrefix, sceneName);
		if (isStatic)
		{
			anyHit = context->createProgramFromPTXString(scenePtx, "any_hit_static");
		}
		else
		{
			anyHit = context->createProgramFromPTXString(scenePtx, "any_hit");
		}
		geometryInstance->getMaterial(0)->setAnyHitProgram(0, anyHit);
	};
	~RigidBody() {};

	void EulerStep(float deltaTime);
	void AddForceAtPosition(float3 force, float3 worldPosition);
	void AddImpulseAtPosition(float3 impulse, float3 worldPosition);
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
	GeometryInstance geometryInstance;

	float transformMatrix[16];

	bool isStatic;
	float kDrag = 0.5f;
	bool useGravity;

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
