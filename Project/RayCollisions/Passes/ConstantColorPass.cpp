/**********************************************************************************************************************
# Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
# following conditions are met:
#  * Redistributions of code must retain the copyright notice, this list of conditions and the following disclaimer.
#  * Neither the name of NVIDIA CORPORATION nor the names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT
# SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************************************************************/

#include "ConstantColorPass.h"

bool ConstantColorPass::initialize(RenderContext* pRenderContext, ResourceManager::SharedPtr pResManager)
{
	// Stash a copy of our resource manager, allowing us to access shared rendering resources
	//    We need an output buffer; tell our resource manager we expect the standard output channel
	mpResManager = pResManager;
	mpResManager->requestTextureResource(ResourceManager::kOutputChannel);

	// Create pipeline state and our fullscreen draw call
	mpGfxState = GraphicsState::create();
	mpCustomPass = FullscreenLaunch::create("Shaders\\custom.hlsl");

    return true;  // Successful initialization.
}

void ConstantColorPass::renderGui(Gui* pGui)
{
	// Add widgets in our options panel to fine tune properties during run-time
	pGui->addFloat3Var("Custom Color", mConstColor, 0.0f, 1.0f);
	pGui->addFloatVar("Sin Intensity", mScaleValue, 0.0f, 1.0f, 0.000001f);
}

void ConstantColorPass::execute(RenderContext* pRenderContext)
{
	// Create a framebuffer object to render to.  Done here once per frame for simplicity, not performance.
	// This function allows us provide a list of managed texture names, which get combined into an FBO
	Fbo::SharedPtr outputFbo = mpResManager->createManagedFbo({ ResourceManager::kOutputChannel });

	// No valid framebuffer?  We're done.
	if (!outputFbo) return;

	// Set shader parameters.  PerFrameCB is a named constant buffer in our HLSL shader
	// "PerFrameCB" is the cbuffer object defined somewhere in our shader file
	auto shaderVars = mpCustomPass->getVars();
	shaderVars["PerFrameCB"]["gFrameCount"] = mFrameCount++;
	shaderVars["PerFrameCB"]["gMultValue"] = mScaleValue;
	shaderVars["PerFrameCB"]["gColor"] = mConstColor;

	// Execute our shader
	mpGfxState->setFbo(outputFbo);
	mpCustomPass->execute(pRenderContext, mpGfxState);
}
