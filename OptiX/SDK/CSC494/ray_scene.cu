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

#include "tutorial.h"
#include "Lights.h"

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

// Camera variables
rtDeclareVariable(float3, eye, , );
rtDeclareVariable(float3, U, , );
rtDeclareVariable(float3, V, , );
rtDeclareVariable(float3, W, , );
rtDeclareVariable(float3, bad_color, , );
rtDeclareVariable(float2, orthoCameraSize, , );

// Output buffers
rtBuffer<uchar4, 2>              output_buffer;
rtBuffer<uchar4, 2>              volume_buffer;

// Volumetric variables
rtDeclareVariable(bool, ignore_intersection, attribute ignore_intersection, );
rtDeclareVariable(float2, t_values, attribute t_values, );

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
rtDeclareVariable(float,   specularPower, , );


// Scene values
rtDeclareVariable(float3, bg_color, , );


//
// Collision volume functions
//

// Checks given a list of entry and exit points if any of them overlap
// indicating that an intersection has occured for the current ray
bool CheckIntersectionOverlap(PerRayData_radiance prd)
{
	float2 screen = make_float2(output_buffer.size());
	float2 pixelSize = orthoCameraSize / screen;
	for (int i = 0; i < prd.numIntersections; i++)
	{
		float2 firstInterval = prd.intersections[i];
		for (int j = i + 1; j < prd.numIntersections; j++)
		{
			float2 secondInterval = prd.intersections[j];
			if (firstInterval.x <= secondInterval.y && secondInterval.x <= firstInterval.y)
			{
				// Compute intersection volume and save it to our buffer
				float volume = (firstInterval.y - secondInterval.x) * pixelSize.x * pixelSize.y;
				float col = volume * screen.x * screen.y * 0.05f; // Compute a relevant color value for the buffer
				volume_buffer[launch_index] = make_color(make_float3(col+0.1, 0, 0));
				return true;
			}
		}
	}
	volume_buffer[launch_index] = prd.numIntersections > 0 ? make_color(make_float3(0.1, 0.1, 0.1)) : make_color(make_float3(0, 0, 0));
	return false;
}


//
// Camera functions
//

// Perspective camera
RT_PROGRAM void perspective_camera()
{
	size_t2 screen = output_buffer.size();

	float2 d = make_float2(launch_index) / make_float2(screen) * 2.f - 1.f;
	float3 ray_origin = eye;
	float3 ray_direction = normalize(d.x*U + d.y*V + W);

	optix::Ray ray(ray_origin, ray_direction, radiance_ray_type, scene_epsilon);

	PerRayData_radiance prd;
	prd.importance = 1.f;
	prd.depth = 0;
	prd.numIntersections = 0;
	prd.closestTval = 999999.0f;

	rtTrace(top_object, ray, prd);

	volume_buffer[launch_index] = make_color(make_float3(0, 0, 0));
	if (prd.numIntersections > 0)
	{
		// Check for intersections (and fill in the intersection buffer)
		CheckIntersectionOverlap(prd);

		// Shade the object with the properties we saved while raycasting
		output_buffer[launch_index] = make_color(prd.result);
	}
	else
	{
		output_buffer[launch_index] = make_color(prd.missColor);
	}
}

// Orthographic camera (easier calculations for intersection volumes)
RT_PROGRAM void orthographic_camera()
{
	size_t2 screen = output_buffer.size();

	float2 d = make_float2(launch_index) / make_float2(screen) * 2.f - 1.f;
	float3 ray_origin = eye + d.x*U*orthoCameraSize.x + d.y*V*orthoCameraSize.y;
	float3 ray_direction = normalize(W);

	optix::Ray ray(ray_origin, ray_direction, radiance_ray_type, scene_epsilon);

	PerRayData_radiance prd;
	prd.importance = 1.f;
	prd.depth = 0;
	prd.numIntersections = 0;
	prd.closestTval = 999999.0f;

	prd.closestShadingNormal = make_float3(0.0, 1.0, 0.0);

	rtTrace(top_object, ray, prd);

	volume_buffer[launch_index] = make_color(make_float3(0, 0, 0));
	if (prd.numIntersections > 0)
	{
		// Check for intersections (and fill in the intersection buffer)
		CheckIntersectionOverlap(prd);

		// Shade the object with the properties we saved while raycasting
		// This is essentially the closest hit shader
		output_buffer[launch_index] = make_color(prd.result);
	}
	else
	{
		output_buffer[launch_index] = make_color(prd.missColor);
	}
}


//
// Raytrace functions
//

// Miss program, stored in ray data and will be used if no intersections
// along our ray were recorded
RT_PROGRAM void miss()
{
	prd_radiance.missColor = bg_color;
}


// Any hit program, store depth value and potential shading properties
RT_PROGRAM void any_hit()
{
	// Record our intersection values
	if (prd_radiance.numIntersections < INTERSECTION_SAMPLES)
	{
		prd_radiance.intersections[prd_radiance.numIntersections] = t_values;
		prd_radiance.numIntersections++;

		// Is this the closest object we have seen so far?
		if (min(t_values.x, t_values.y) < prd_radiance.closestTval)
		{
			// Update shading properties since this is now the closest object
			prd_radiance.closestTval = min(t_values.x, t_values.y);
			prd_radiance.closestShadingNormal = normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, shading_normal));
		}
	}

	if (ignore_intersection)
		rtIgnoreIntersection();
}

// Closest hit shading for the spheres
RT_PROGRAM void closest_hit_radiance_sphere()
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

	prd_radiance.result = color;
}

RT_PROGRAM void any_hit_shadow()
{
    // Opaque shadow caster
    prd_shadow.attenuation = 0.0f;

    rtTerminateRay();
}

// Closest hit shading for the plane 
RT_PROGRAM void closest_hit_radiance_plane()
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

	prd_radiance.result = color;
}

// Closest hit shading for the spheres
RT_PROGRAM void closest_hit_radiance_box()
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

	prd_radiance.result = color;
}

// Exception program, deafult to some known exception color
RT_PROGRAM void exception()
{
	output_buffer[launch_index] = make_color(bad_color);
}
