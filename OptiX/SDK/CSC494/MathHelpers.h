#pragma once

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

using namespace optix;

class MathHelpers
{
public:

	static inline float GetMagnitude(float3 vector)
	{
		return sqrt(vector.x*vector.x + vector.y*vector.y + vector.z*vector.z);
	}

	static inline Matrix3x3 QuaternionToRotation(float4 quaternion)
	{
		normalize(quaternion);
		float temp[9];
		temp[0] = 1.0f - 2*quaternion.z*quaternion.z - 2*quaternion.w*quaternion.w;
		temp[1] = 2*quaternion.y*quaternion.z - 2*quaternion.x*quaternion.w;
		temp[2] = 2*quaternion.y*quaternion.w + 2*quaternion.x*quaternion.z;
		temp[3] = 2*quaternion.y*quaternion.z + 2*quaternion.x*quaternion.w;
		temp[4] = 1.0f - 2*quaternion.y*quaternion.y - 2*quaternion.w*quaternion.w;
		temp[5] = 2*quaternion.z*quaternion.w - 2*quaternion.x*quaternion.y;
		temp[6] = 2*quaternion.y*quaternion.w - 2*quaternion.x*quaternion.z;
		temp[7] = 2*quaternion.z*quaternion.w + 2*quaternion.x*quaternion.y;
		temp[8] = 1.0f - 2*quaternion.y*quaternion.y - 2*quaternion.z*quaternion.z;
		Matrix3x3 rotation(temp);
		return rotation;
	}

	static inline float4 RotationToQuaternion(Matrix3x3 rotation)
	{
		float4 quaternion = make_float4(0.0f);
		float trace;
		float s;
		trace = rotation[0] + rotation[4] + rotation[8];

		if (trace >= 0)
		{
			s = sqrt(trace + 1.0f);
			quaternion.x = 0.5f * s;
			s = 0.5 / s;
			quaternion.y = (rotation[7] - rotation[5]) * s;
			quaternion.z = (rotation[2] - rotation[6]) * s;
			quaternion.w = (rotation[3] - rotation[1]) * s;
		}
		else
		{
			int i = 0;

			if(rotation[4] > rotation[0])
				i = 1;

			if(rotation[8] > rotation[i*3+i])
				i = 2;

			switch (i)
			{
				case 0:
					s = sqrt((rotation[0] - (rotation[4] + rotation[8])) + 1.0f);
					quaternion.y = 0.5f * s;
					s = 0.5f / s;
					quaternion.z = (rotation[1] + rotation[3]) * s;
					quaternion.w = (rotation[6] + rotation[2]) * s;
					quaternion.x = (rotation[7] - rotation[5]) * s;
					break;
				case 1:
					s = sqrt((rotation[4] - (rotation[8] + rotation[0])) + 1.0f);
					quaternion.z = 0.5f * s;
					s = 0.5f / s;
					quaternion.w = (rotation[5] + rotation[7]) * s;
					quaternion.y = (rotation[1] + rotation[3]) * s;
					quaternion.x = (rotation[2] - rotation[6]) * s;
					break;
				case 2:
					s = sqrt((rotation[8] - (rotation[0] + rotation[4])) + 1.0f);
					quaternion.w = 0.5f * s;
					s = 0.5f / s;
					quaternion.y = (rotation[6] + rotation[2]) * s;
					quaternion.z = (rotation[5] + rotation[7]) * s;
					quaternion.x = (rotation[3] - rotation[1]) * s;
			}
		}
		return quaternion;
	}
};
