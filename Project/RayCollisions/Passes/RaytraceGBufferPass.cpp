#include "RaytraceGBufferPass.h"

namespace 
{
	// Entry points for the shader
	const char* kEntryPointRayGen = "GBufferRayGen";
	const char* kEntryPointMiss0 = "PrimaryMiss";
	const char* kEntryPrimaryAnyHit = "PrimaryAnyHit";
	const char* kEntryPrimaryClosestHit = "PrimaryClosestHit";
};

bool RaytraceGBufferPass::initialize(RenderContext* pRenderContext, ResourceManager::SharedPtr pResManager)
{
	mpResManager = pResManager;
	mpResManager->requestTextureResources({ "MaterialDiffuse", "WorldPosition", "WorldNormal", "MaterialSpecular", "Emissive" });

	// Setup shader entry points
	mpRays = RayLaunch::create("Shaders\\raytraceGBuffer.hlsl", kEntryPointRayGen);
	mpRays->addMissShader("Shaders\\raytraceGBuffer.hlsl", kEntryPointMiss0);                             // Init miss shader
	mpRays->addHitShader("Shaders\\raytraceGBuffer.hlsl", kEntryPrimaryClosestHit, kEntryPrimaryAnyHit);  // Init hit shaders

	mpRays->compileRayProgram();
	if (mpScene) mpRays->setScene(mpScene);

	return true;
}

void RaytraceGBufferPass::initScene(RenderContext* pRenderContext, Scene::SharedPtr pScene)
{
	// Currently, raytracing requires a RtScene which is derived from Scene. Everything loaded that gets
	// passed into here will be an RtScene, so just cast it to one.
	mpScene = std::dynamic_pointer_cast<RtScene>(pScene);

	if (mpRays) mpRays->setScene(mpScene);
}

void RaytraceGBufferPass::execute(RenderContext* pRenderContext)
{
	if (!mpRays || !mpRays->readyToRender()) return;

	Texture::SharedPtr matDif = mpResManager->getClearedTexture("MaterialDiffuse", vec4(0, 0, 0, 0));
	Texture::SharedPtr wsPos = mpResManager->getClearedTexture("WorldPosition", vec4(0, 0, 0, 0));
	Texture::SharedPtr wsNorm = mpResManager->getClearedTexture("WorldNormal", vec4(0, 0, 0, 0));
	Texture::SharedPtr matSpec = mpResManager->getClearedTexture("MaterialSpecular", vec4(0, 0, 0, 0));
	Texture::SharedPtr matEmit = mpResManager->getClearedTexture("Emissive", vec4(0, 0, 0, 0));

	// Pass variables down to our shader
	auto missVars = mpRays->getMissVars(0);
	missVars["MissShaderCB"]["gBgColor"] = mBgColor;
	missVars["gMatDif"] = matDif;

	for (auto pVars : mpRays->getHitVars(0))
	{
		pVars["gMatDif"] = matDif;
		pVars["gWsPos"] = wsPos;
		pVars["gWsNorm"] = wsNorm;
		pVars["gMatSpec"] = matSpec;
		pVars["gMatEmissive"] = matEmit;
	}

	// Raytrace!
	mpRays->execute(pRenderContext, mpResManager->getScreenSize());
}
