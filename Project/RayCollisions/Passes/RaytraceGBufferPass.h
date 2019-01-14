#pragma once
#include "../SharedUtils/RenderPass.h"
#include "../SharedUtils/SimpleVars.h"
#include "../SharedUtils/RayLaunch.h"

class RaytraceGBufferPass : public ::RenderPass, inherit_shared_from_this<::RenderPass, RaytraceGBufferPass>
{
public:
	using SharedPtr = std::shared_ptr<RaytraceGBufferPass>;

	static SharedPtr create() { return SharedPtr(new RaytraceGBufferPass()); }
	virtual ~RaytraceGBufferPass() = default;

protected:
	RaytraceGBufferPass() : ::RenderPass("Ray Traced G-Buffer", "Ray Traced G-Buffer Options") {}

	bool initialize(RenderContext* pRenderContext, ResourceManager::SharedPtr pResManager) override;
	void execute(RenderContext* pRenderContext) override;
	void initScene(RenderContext* pRenderContext, Scene::SharedPtr pScene) override;

	bool requiresScene() override { return true; }
	bool usesRayTracing() override { return true; } // Uses real-time raytracing

	RayLaunch::SharedPtr        mpRays;
	RtScene::SharedPtr          mpScene; // Raytracing uses a specific scene wrapper

	vec3                        mBgColor = vec3(0.5f, 0.5f, 1.0f); // Miss colour if no geometery is render for this pixel
};
