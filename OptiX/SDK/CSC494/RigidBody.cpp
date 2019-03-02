// OptiX
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include <sutil.h>

#include "RigidBody.h"

using namespace optix;

// Update our rigid bodies data by computing a physics update
void RigidBody::EulerStep(float deltaTime)
{
	// Compute derivatives
	float3 positionDeriv = velocity;
	float3 velocityDeriv = forceAccumulation / mass;

	// Scale relative to our time step
	positionDeriv *= deltaTime;
	velocityDeriv *= deltaTime;

	// Update our rigidbody using the discrete derivative step
	position += positionDeriv;
	velocity += velocityDeriv;

	// Update our mesh's position
	mesh["position"]->setFloat(position);

	// Zero out force accumulation since we've passed it off into the velocity now
	forceAccumulation = make_float3(0.0f, 0.0f, 0.0f);

	ApplyDrag();
}

void RigidBody::ApplyDrag()
{
	velocity -= velocity * kDrag;
}
