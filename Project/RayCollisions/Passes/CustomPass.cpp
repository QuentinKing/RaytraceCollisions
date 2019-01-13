#include "CustomPass.h"

bool CustomPass::initialize(RenderContext* pRenderContext, ResourceManager::SharedPtr pResManager)
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

void CustomPass::renderGui(Gui* pGui)
{
	// Add widgets in our options panel to fine tune properties during run-time
	pGui->addFloat3Var("Custom Color", mConstColor, 0.0f, 1.0f);
	pGui->addFloatVar("Sin Intensity", mScaleValue, 0.0f, 1.0f, 0.000001f);
}

void CustomPass::execute(RenderContext* pRenderContext)
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
