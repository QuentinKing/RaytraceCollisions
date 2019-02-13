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

rtDeclareVariable(PerRayData_radiance, prd_radiance, rtPayload, );

rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );
rtDeclareVariable(uint2, launch_index, rtLaunchIndex, );

rtDeclareVariable(unsigned int, radiance_ray_type, , );
rtDeclareVariable(float, scene_epsilon, , );
rtDeclareVariable(rtObject, top_object, , );


//
// Pinhole camera implementation
//
rtDeclareVariable(float3, eye, , );
rtDeclareVariable(float3, U, , );
rtDeclareVariable(float3, V, , );
rtDeclareVariable(float3, W, , );
rtDeclareVariable(float3, bad_color, , );
rtDeclareVariable(float2, orthoCameraSize, , );
rtBuffer<uchar4, 2>              output_buffer;

// Volumetric variables
rtDeclareVariable(float3, shading_normal, attribute shading_normal, );
rtDeclareVariable(float2, t_values, attribute t_values, );

// Checks given a list of entry and exit points if any of them overlap
// indicating that an intersection has occured for the current ray
bool CheckIntersectionOverlap(PerRayData_radiance prd)
{
	for (int i = 0; i < prd.numIntersections; i++)
	{
		float2 firstInterval = prd.intersections[i];
		for (int j = i + 1; j < prd.numIntersections; j++)
		{
			float2 secondInterval = prd.intersections[j];
			if (firstInterval.x <= secondInterval.y && secondInterval.x <= firstInterval.y)
			{
				return true;
			}
		}
	}
	return false;
}

// Perspective camera (Not in use currently)
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

	rtTrace(top_object, ray, prd);

	output_buffer[launch_index] = make_color(prd.result);
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

	rtTrace(top_object, ray, prd);

	if (prd.numIntersections > 0)
	{
		output_buffer[launch_index] = CheckIntersectionOverlap(prd) ? make_color(make_float3(0, 0, 0)) : make_color(make_float3(1, 1, 1));
	}
	else
	{
		output_buffer[launch_index] = make_color(prd.result);
	}
}

//
// Returns solid color for miss rays
//
rtDeclareVariable(float3, bg_color, , );
RT_PROGRAM void miss()
{
	prd_radiance.result = bg_color;
}


//
// Returns shading normal as the surface shading result
// 
RT_PROGRAM void closest_hit_radiance0()
{
	prd_radiance.result = normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, shading_normal))*0.5f + 0.5f;
}

//
// Returns shading normal as the surface shading result
// 
RT_PROGRAM void closest_hit_radiance1()
{
	prd_radiance.result = make_float3(1.0f, 1.0f, 1.0f);
}

// Any hit program
RT_PROGRAM void any_hit()
{
	// Record our intersection values
	if (prd_radiance.numIntersections < INTERSECTION_SAMPLES)
	{
		prd_radiance.intersections[prd_radiance.numIntersections] = t_values;
		prd_radiance.numIntersections++;
	}

	rtIgnoreIntersection();
}

//
// Set pixel to solid color upon failur
//
RT_PROGRAM void exception()
{
	output_buffer[launch_index] = make_color(bad_color);
}
