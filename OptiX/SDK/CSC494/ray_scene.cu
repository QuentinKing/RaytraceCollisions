/*
 * Copyright (c) 2018 NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
rtBuffer<uchar4, 2>								 output_buffer;
rtBuffer<IntersectionResponse, 2>				 collisionResponse;

// Rigidbody variables
rtDeclareVariable(int, numRigidbodies, , );
rtDeclareVariable(int, physicsRayStep, , );
rtDeclareVariable(int, physicsBufferWidth, , );
rtDeclareVariable(int, physicsBufferHeight, , );
rtBuffer<RigidbodyMotion> rigidbodyMotions; 

// Volumetric variables
rtDeclareVariable(IntersectionData, intersectionData, attribute intersectionData, );
rtDeclareVariable(float, staticTVal, attribute staticTVal, );
rtDeclareVariable(bool, ignore_intersection, attribute ignore_intersection, );

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

// Checks given a list of entry and exit points if any of them overlap
// indicating that an intersection has occured for the current ray
void CheckIntersectionOverlap(PerRayData_radiance prd, float3 ray_origin, float3 ray_direction)
{
	float2 screen = make_float2(output_buffer.size());
	float total = 0.0f;

	for (int i = 0; i < prd.numIntersections; i++)
	{
		float2 firstInterval = make_float2(prd.intersections[i].entryTval, prd.intersections[i].exitTval);
		for (int j = i + 1; j < prd.numIntersections; j++)
		{
			float2 secondInterval = make_float2(prd.intersections[j].entryTval, prd.intersections[j].exitTval);

			// Compute intersection volume and save it to our buffer
			float intersection = max(0.0f, min(firstInterval.y, secondInterval.y) - max(firstInterval.x, secondInterval.x));
			int entryIndex = max(firstInterval.x, secondInterval.x) == firstInterval.x ? i : j;
			int exitIndex = min(firstInterval.y, secondInterval.y) == firstInterval.y ? i : j;

			float fovDelta = 1.0 / screen.x;
			float theta = fov * fovDelta;
			float phi = 90.0 - theta;
			float a = sin(theta) * prd.intersections[entryIndex].entryTval / sin(phi);
			float b = sin(theta) * prd.intersections[exitIndex].exitTval / sin(phi);
			float h = intersection;
			float volume = 0.33 * (a*a + a * b + b * b) * h;

			IntersectionResponse data;
			data.volume = volume;
			data.entryId = prd.intersections[entryIndex].rigidBodyId;
			data.entryNormal = prd.intersections[entryIndex].entryNormal;
			data.exitId = prd.intersections[exitIndex].rigidBodyId;
			data.exitNormal = prd.intersections[exitIndex].exitNormal;
			data.entryPoint = ray_origin + prd.intersections[entryIndex].entryTval * ray_direction;
			data.exitPoint = ray_origin + prd.intersections[exitIndex].exitTval * ray_direction;
			collisionResponse[make_uint2(launch_index.x / physicsRayStep, launch_index.y / physicsRayStep)] = data;

			total += volume;
		}
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

	optix::Ray ray(ray_origin, ray_direction, radiance_ray_type, scene_epsilon);

	PerRayData_radiance prd;
	prd.importance = 1.f;
	prd.depth = 0;
	prd.numIntersections = 0;
	prd.hitObject = false;
	prd.closestTval = 999999.0f;

	rtTrace(top_object, ray, prd);

	if (prd.hitObject)
	{
		// Check for intersections (and fill in the intersection buffer)
		if (isPhysicsRay)
			CheckIntersectionOverlap(prd, ray_origin, ray_direction);
	}

	output_buffer[launch_index] = make_color(prd.result);
}

// Closest hit shading for the spheres
RT_PROGRAM void closest_hit_radiance()
{
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
 
	float3 hit_point = ray.origin + closestHitDist * ray.direction;

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
		float3 R = reflect(ray.direction, ffnormal);
		optix::Ray refl_ray( hit_point, R, radiance_ray_type, scene_epsilon );
		rtTrace(top_object, refl_ray, refl_prd);
		color += r * refl_prd.result;
	}

	prd_radiance.result = color;
}

// Any hit program, store depth value and potential shading properties
RT_PROGRAM void any_hit()
{
	prd_radiance.hitObject = true;

	// Record our intersection values
	if (prd_radiance.numIntersections < INTERSECTION_SAMPLES)
	{
		prd_radiance.intersections[prd_radiance.numIntersections] = intersectionData;
		prd_radiance.numIntersections++;

		// Is this the closest object we have seen so far?
		if (intersectionData.entryTval < prd_radiance.closestTval)
		{
			// Update shading properties since this is now the closest object
			prd_radiance.closestTval = intersectionData.entryTval;
		}
	}

	if (ignore_intersection)
		rtIgnoreIntersection();
}

RT_PROGRAM void any_hit_static()
{
	// Record our intersection values
	prd_radiance.hitObject = true;
	prd_radiance.closestTval = min(prd_radiance.closestTval, staticTVal);
}

// Miss program, stored in ray data and will be used if no intersections
// along our ray were recorded
RT_PROGRAM void miss()
{
	float3 point = normalize(ray.direction);
	float u = atan2(point.x, point.z) / (2.0 * M_PIf) + 0.5;
	float v = point.y * 0.5 + 0.5;
	prd_radiance.result = make_float3(tex2D(envmap, u, v));
}

RT_PROGRAM void any_hit_shadow()
{
    // Opaque shadow caster
    prd_shadow.attenuation = 0.0f;

    rtTerminateRay();
}

// Exception program, deafult to some known exception color
RT_PROGRAM void exception()
{
	const unsigned int code = rtGetExceptionCode();
	rtPrintf("Exception code 0x%X at (%d, %d)\n", code, launch_index.x, launch_index.y);
	output_buffer[launch_index] = make_color(bad_color);
}

