#include "RayStructs.h"
#include "BufferStructs.h"

// Ray data
rtDeclareVariable(PerRayData_radiance, prd_radiance, rtPayload, );
rtDeclareVariable(PerRayData_shadow,   prd_shadow,   rtPayload, );
rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );
rtDeclareVariable(uint2, launch_index, rtLaunchIndex, );
rtDeclareVariable(unsigned int, radiance_ray_type, , );
rtDeclareVariable(unsigned int, shadow_ray_type , , );
rtDeclareVariable(float, scene_epsilon, , );
rtDeclareVariable(rtObject, top_object, , );
rtDeclareVariable(rtObject, top_shadower, , );
rtDeclareVariable(float, closestHitDist, rtIntersectionDistance, );
rtDeclareVariable(float, importance_cutoff, , );
rtDeclareVariable(int, max_depth, , );

// Camera variables
rtDeclareVariable(float3, eye, , );
rtDeclareVariable(float3, U, , );
rtDeclareVariable(float3, V, , );
rtDeclareVariable(float3, W, , );
rtDeclareVariable(float3, bad_color, , );
rtDeclareVariable(float, fov, , );

// Output buffers
rtBuffer<uchar4, 2> output_buffer;
rtBuffer<IntersectionResponse, 2> collisionResponse;

// Rigidbody variables
rtDeclareVariable(int, physicsRayStep, , );
rtDeclareVariable(int, physicsBufferWidth, , );
rtDeclareVariable(int, physicsBufferHeight, , );

// Volumetric variables
rtDeclareVariable(IntersectionData, intersectionData, attribute intersectionData, );

// Shading values
rtDeclareVariable(float3, shading_normal, attribute shading_normal, );
rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, );

// Light values
rtDeclareVariable(float3, ambientLightColor, , );
rtBuffer<Light> lights; 

// Material values
rtDeclareVariable(float3, ambientColorIntensity, , );
rtDeclareVariable(float3, diffuseColorIntensity, , );
rtDeclareVariable(float3, specularColorIntensity, , );
rtDeclareVariable(float3, fresnel, , );
rtDeclareVariable(float3, reflectivity, , );
rtDeclareVariable(float,   specularPower, , );

// Scene values
rtTextureSampler<float4, 2> envmap;

void ClearResponseBuffer()
{
	IntersectionResponse data;
	data.volume = 0.0f;
	data.entryId = 0;
	data.entryNormal = make_float3(0.0f,0.0f,0.0f);
	data.exitId = 0;
	data.exitNormal = make_float3(0.0f,0.0f,0.0f);
	data.entryPoint = make_float3(0.0f,0.0f,0.0f);
	data.exitPoint = make_float3(0.0f,0.0f,0.0f);
	collisionResponse[make_uint2(launch_index.x / physicsRayStep, launch_index.y / physicsRayStep)] = data;
}

// Given an ordered list of ray intersections, finds all intervals of intersections and
// computes their volumes.
void CheckIntersectionOverlap(PerRayData_radiance prd, float3 ray_origin, float3 ray_direction, float3& result)
{
	// Right now, just store the largest intersection volume into the buffer
	IntersectionResponse largestResponse;
	largestResponse.volume = 0.0f;
	largestResponse.entryId = 0;
	largestResponse.entryNormal = make_float3(0.0, 0.0, 0.0);
	largestResponse.exitId = 0;
	largestResponse.exitNormal = make_float3(0.0, 0.0, 0.0);
	largestResponse.entryPoint = make_float3(0.0, 0.0, 0.0);
	largestResponse.exitPoint = make_float3(0.0, 0.0, 0.0);
	largestResponse.collisionId = 0;
	IntersectionData objectsInside[5]; // Assume we will never have more than 5 bodies intersecting at any given point
	int insideIndex = 0;

	float2 screen = make_float2(output_buffer.size());
	float fovDelta = 1.0 / screen.x;
	float theta = fov * fovDelta;
	float phi = 90.0 - theta;

	// Debugging
	float largestT = 0.0;

	for (int i = 0; i < prd.numIntersections; i++)
	{
		IntersectionData objEnter; // Will be set if we need it

		// Check to see if we are entering this object
		bool entering = true;
		for (int j = 0; j < insideIndex; j++)
		{
			if (objectsInside[j].rigidBodyId == prd.intersections[i].rigidBodyId)
			{
				objEnter = objectsInside[j];
				entering = false;
			}
		}

		// If entering, add it to our tracking array
		if (entering)
		{
			// Check if this intersection has an exit point (ie, is valid)
			bool isValid = false;
			for (int j = i + 1; j < prd.numIntersections; j++)
			{
				if (prd.intersections[j].rigidBodyId == prd.intersections[i].rigidBodyId)
				{
					isValid = true;
					break;
				}
			}

			if (isValid)
			{
				objectsInside[insideIndex] = prd.intersections[i];
				insideIndex++;
			}
		}
		else
		{
			// Otherwise, we are exiting this object, need to check for intersection volumes
			// with any other objects we are currently inside
			for (int j = 0; j < insideIndex; j++)
			{
				if (objectsInside[j].rigidBodyId != prd.intersections[i].rigidBodyId)
				{
					// Compute volume
					IntersectionData entryPoint = objectsInside[j].t < objEnter.t ? objEnter : objectsInside[j];
					IntersectionData exitPoint = prd.intersections[i];
					
					float a = sin(theta) * entryPoint.t/ sin(phi);
					float b = sin(theta) * exitPoint.t/ sin(phi);
					float h = exitPoint.t - entryPoint.t;
					float volume = 0.33 * (a*a + a * b + b * b) * h;

					if (volume > largestResponse.volume)
					{
						largestResponse.volume = volume;
						largestResponse.entryId = entryPoint.rigidBodyId;
						largestResponse.entryNormal = entryPoint.normal;
						largestResponse.exitId = exitPoint.rigidBodyId;
						largestResponse.exitNormal = exitPoint.normal;
						largestResponse.entryPoint = ray_origin + entryPoint.t * ray_direction;
						largestResponse.exitPoint = ray_origin + exitPoint.t * ray_direction;
						largestResponse.collisionId = objectsInside[j].rigidBodyId;
					}
				}
			}

			// Remove this object from our tracking array
			bool shift = false;
			for (int j = 0; j < insideIndex; j++)
			{
				if (objectsInside[j].rigidBodyId == prd.intersections[i].rigidBodyId)
				{
					shift = true;
					continue;
				}

				if (shift)
				{
					objectsInside[j - 1] = objectsInside[j];
				}
			}
			insideIndex--;
		}

		// Finally, assign our biggest response to the buffer
		collisionResponse[make_uint2(launch_index.x / physicsRayStep, launch_index.y / physicsRayStep)] = largestResponse;
		
	}
}

RT_PROGRAM void perspective_camera()
{
	// Determine if we are going to use this ray for volume intersections
	bool isPhysicsRay = (launch_index.x % physicsRayStep == 0 && launch_index.y % physicsRayStep == 0);

	if (isPhysicsRay)
		ClearResponseBuffer();

	size_t2 screen = output_buffer.size();

	float2 d = make_float2(launch_index) / make_float2(screen) * 2.f - 1.f;
	float3 ray_origin = eye;
	float3 ray_direction = normalize(d.x*U + d.y*V + W);

	float3 result = make_float3( 0.0f );


	PerRayData_radiance prd;
	prd.physicsRay = isPhysicsRay;
	prd.done = false;

	prd.origin = eye;

	prd.result = make_float3(0.0, 0.0, 0.0);
	prd.importance = 1.0;
	prd.depth = 0;

	prd.numIntersections = 0;

	for (;;)
	{
		optix::Ray ray(ray_origin, ray_direction, radiance_ray_type, scene_epsilon);
		rtTrace(top_object, ray, prd);
		result += prd.result;

		if (prd.done || !isPhysicsRay)
		{
			break;
		}

		prd.depth++;
		ray_origin = prd.origin;
	}

	if (isPhysicsRay)
		CheckIntersectionOverlap(prd, eye, ray_direction, result);

	output_buffer[launch_index] = make_color(result);
}

// Closest hit shading for the spheres
RT_PROGRAM void closest_hit_radiance()
{
	float3 hit_point = ray.origin + closestHitDist * ray.direction;
	float3 new_origin = ray.origin + (closestHitDist + 0.001) * ray.direction;
	prd_radiance.origin = new_origin;

	IntersectionData data;
	data.rigidBodyId = intersectionData.rigidBodyId;
	data.t = prd_radiance.numIntersections == 0 ? intersectionData.t : intersectionData.t + prd_radiance.intersections[prd_radiance.numIntersections - 1].t;
	data.normal = intersectionData.normal;

	prd_radiance.intersections[prd_radiance.numIntersections] = data;
	prd_radiance.numIntersections++;

	// Only shade first object we come in contact with, so no transparency
	// for now, but possible to implement later
	if (prd_radiance.numIntersections != 1)
	{
		prd_radiance.result = make_float3(0.0, 0.0, 0.0);
		return;
	}

	float3 world_geo_normal = normalize(rtTransformNormal(
										RT_OBJECT_TO_WORLD,
										geometric_normal));

	float3 world_shade_normal = normalize(rtTransformNormal(
											RT_OBJECT_TO_WORLD,
											shading_normal));

	// Handles back face rendering
	float3 ffnormal = faceforward(world_shade_normal,
									-ray.direction,
									world_geo_normal);

	float3 color = ambientColorIntensity * ambientLightColor;

	// Phong diffuse shading
	for(int i = 0; i < lights.size(); ++i) 
	{
		Light light = lights[i];
		float3 L = normalize(light.pos - hit_point);
		float nDl = __saturatef(dot( ffnormal, L));
		
		if( nDl > 0 )
		{
			// Cast a shadow ray
			PerRayData_shadow shadow_prd;
			shadow_prd.attenuation = 1.0f;
			float Ldist = length(light.pos - hit_point);
			optix::Ray shadow_ray(hit_point, L, shadow_ray_type, scene_epsilon, Ldist );
			rtTrace(top_shadower, shadow_ray, shadow_prd);
			float light_attenuation = shadow_prd.attenuation;

			if (light_attenuation > 0.0f)
			{
				float3 Lc = light.color * light_attenuation;
				color += diffuseColorIntensity * nDl * Lc;

				float3 H = normalize(L - ray.direction); // half way vector
				float nDh = dot(ffnormal, H);
				if (nDh > 0)
					color += specularColorIntensity * Lc * pow(nDh, specularPower);
			}
			else
			{
				return;
			}
		}
	}

	float3 r = schlick(-dot(ffnormal, ray.direction), fresnel);
	float importance = prd_radiance.importance * optix::luminance(reflectivity);

	// reflection ray
	if (importance > importance_cutoff && prd_radiance.depth < max_depth) 
	{
		PerRayData_radiance refl_prd;
		refl_prd.importance = importance;
		refl_prd.depth = prd_radiance.depth+1;

		refl_prd.physicsRay = false;

		refl_prd.result = make_float3(0.0, 0.0, 0.0);
		refl_prd.importance = importance;
		refl_prd.depth = prd_radiance.depth+1;

		refl_prd.numIntersections = 0;

		float3 R = reflect(ray.direction, ffnormal);
		optix::Ray refl_ray( hit_point + 0.001 * R, R, radiance_ray_type, scene_epsilon );
		rtTrace(top_object, refl_ray, refl_prd);
		color += r * refl_prd.result;
	}

	prd_radiance.result = color;
}

// Miss program, stored in ray data and will be used if no intersections
// along our ray were recorded
RT_PROGRAM void miss()
{
	// No more things to raytrace!
	prd_radiance.done = true;

	float3 point = normalize(ray.direction);
	float u = atan2(point.x, point.z) / (2.0 * M_PIf) + 0.5;
	float v = point.y * 0.5 + 0.5;

	if (prd_radiance.numIntersections == 0)
	{
		prd_radiance.result = make_float3(tex2D(envmap, u, v));
	}
	else
	{
		prd_radiance.result = make_float3(0.0, 0.0, 0.0);
	}
}

RT_PROGRAM void any_hit_shadow()
{
    // Opaque shadow caster
    prd_shadow.attenuation = 0.0f;

    rtTerminateRay();
}

// Exception program, default to some known exception color
RT_PROGRAM void exception()
{
	const unsigned int code = rtGetExceptionCode();
	rtPrintf("Exception code 0x%X at (%d, %d)\n", code, launch_index.x, launch_index.y);
	output_buffer[launch_index] = make_color(bad_color);
}

