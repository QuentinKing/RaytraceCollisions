
#include "VertexAttrib.h"
__import ShaderCommon;
__import DefaultVS;

// Default vertex shader
VertexOut main(VertexIn vIn)
{
	return defaultVS(vIn);
}
