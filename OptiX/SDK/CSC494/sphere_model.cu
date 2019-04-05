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

#include <optix_world.h>
#include "tutorial.h"

using namespace optix;

// Volumetric variables (All geometry need this)
rtDeclareVariable(IntersectionData, intersectionData, attribute intersectionData, );
rtDeclareVariable(bool, ignore_intersection, attribute ignore_intersection, );
rtDeclareVariable(float, current_closest, rtIntersectionDistance, );
rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );

// Rigidbody specific variables
rtDeclareVariable(float, id, , );

// Sphere specific variables
rtDeclareVariable(float, radius, , );

// Shading variables (Technically not required, but usually used on all materials)
rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, );
rtDeclareVariable(float3, shading_normal, attribute shading_normal, );


template<bool use_robust_method>
static __device__
void intersect_sphere(void)
{
	float3 O = ray.origin;
	float3 D = ray.direction;

	float b = dot(O, D);
	float c = dot(O, O) - radius * radius;
	float disc = b * b - c;
	if (disc > 0.0f) {
		float sdisc = sqrtf(disc);
		float root1 = (-b - sdisc);

		bool do_refine = false;

		float root11 = 0.0f;

		if (use_robust_method && fabsf(root1) > 10.f * radius) 
		{
			do_refine = true;
		}

		if (do_refine) 
		{
			// refine root1
			float3 O1 = O + root1 * ray.direction;
			b = dot(O1, D);
			c = dot(O1, O1) - radius * radius;
			disc = b * b - c;

			if (disc > 0.0f) {
				sdisc = sqrtf(disc);
				root11 = (-b - sdisc);
			}
		}

		float t1 = root1 + root11;
		float t2 = (-b + sdisc) + (do_refine ? root1 : 0);

		// Always call the any hit function, so we have to report an intersection closer than 
		// the closest intersection. If we have to fudge the numbers a bit to make sure we call the any-hit
		// function, make sure we ignore the intersection so it doesn't store this value.
		bool ignore = t1 > current_closest; 
		float modified_t_value = ignore ? current_closest - 1.0f : t1;

		if (rtPotentialIntersection(modified_t_value))
		{
			ignore_intersection = ignore;

			IntersectionData data;
			data.rigidBodyId = id;
			data.entryTval = t1;
			data.exitTval = t2;
			data.entryNormal = (O + (root1 + root11)*D) / radius;
			data.exitNormal = (O + t2*D)/radius;
			intersectionData = data;

			shading_normal = geometric_normal = data.entryNormal;
			rtReportIntersection(0);
		}
	}
}

RT_PROGRAM void intersect(int primIdx)
{
	intersect_sphere<false>();
}


RT_PROGRAM void robust_intersect(int primIdx)
{
	intersect_sphere<true>();
}


RT_PROGRAM void bounds(int, float result[6])
{
	const float3 rad = make_float3(radius);

	optix::Aabb* aabb = (optix::Aabb*)result;

	if (rad.x > 0.0f && !isinf(rad.x)) {
		aabb->m_min = rad;
		aabb->m_max = rad;
	}
	else {
		aabb->invalidate();
	}
}

