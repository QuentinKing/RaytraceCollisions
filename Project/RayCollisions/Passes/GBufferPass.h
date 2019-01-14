#pragma once
#include "../SharedUtils/RenderPass.h"
#include "../SharedUtils/RasterLaunch.h"

class GBufferPass : public ::RenderPass, inherit_shared_from_this<::RenderPass, GBufferPass>
{
public:
    using SharedPtr = std::shared_ptr<GBufferPass>;

    static SharedPtr create() { return SharedPtr(new GBufferPass()); }
    virtual ~GBufferPass() = default;

protected:
	GBufferPass() : ::RenderPass("G-Buffer Creation", "G-Buffer Options") {}

    bool initialize(RenderContext* pRenderContext, ResourceManager::SharedPtr pResManager) override;
    void execute(RenderContext* pRenderContext) override;
	void initScene(RenderContext* pRenderContext, Scene::SharedPtr pScene) override;

	bool requiresScene() override { return true; } // Requires a 3D environment to render
	bool usesRasterization() override { return true; } // Uses rasterization

    GraphicsState::SharedPtr    mpGfxState;             // Current graphics pipeline state
	Scene::SharedPtr            mpScene;                // Pointer to the scene we're rendering
	RasterLaunch::SharedPtr     mpRaster;               // Wrapper managing the shader for our g-buffer creation

	vec3                        mBgColor = vec3(0.5f, 0.5f, 1.0f);  // Miss colour if no geometery is render for this pixel
};
