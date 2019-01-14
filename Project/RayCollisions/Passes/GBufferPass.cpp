#include "GBufferPass.h"

bool GBufferPass::initialize(RenderContext* pRenderContext, ResourceManager::SharedPtr pResManager)
{
	// For our resource manager, identity the channels we are writing to
	mpResManager = pResManager;
	mpResManager->requestTextureResources({ "WorldPosition", "WorldNormal", "MaterialSpecular", "MaterialDiffuse"});
	mpResManager->requestTextureResource("Z-Buffer", ResourceFormat::D24UnormS8, ResourceManager::kDepthBufferFlags);

	// Default scene, taken from the data from the NVIDIA tutorials
	mpResManager->setDefaultSceneName("Data\\pink_room\\pink_room.fscene");

	mpGfxState = GraphicsState::create();

	mpRaster = RasterLaunch::createFromFiles("Shaders\\gBufferVert.hlsl", "Shaders\\gBufferFrag.hlsl");
	mpRaster->setScene(mpScene);

	return true;
}

void GBufferPass::initScene(RenderContext* pRenderContext, Scene::SharedPtr pScene)
{
	if (pScene)
		mpScene = pScene;

	if (mpRaster)
		mpRaster->setScene(mpScene);
}

void GBufferPass::execute(RenderContext* pRenderContext)
{
	// Create frame buffer
	Fbo::SharedPtr outputFbo = mpResManager->createManagedFbo(
		{ "WorldPosition", "WorldNormal", "MaterialSpecular", "MaterialDiffuse" }, // Color Buffers
		"Z-Buffer");															  // Depth Buffers

	if (!outputFbo) 
		return;

	// Clear g-buffer.  Clear colors to black, depth to 1, stencil to 0, but then clear diffuse texture to our bg color
	pRenderContext->clearFbo(outputFbo.get(), vec4(0, 0, 0, 0), 1.0f, 0);
	pRenderContext->clearUAV(outputFbo->getColorTexture(3)->getUAV().get(), vec4(mBgColor, 1.0f));

	mpRaster->execute(pRenderContext, mpGfxState, outputFbo);
}
