#pragma once
#include "../SharedUtils/RenderingPipeline.h"

class CollisionPipeline : public ::RenderingPipeline, inherit_shared_from_this<::RenderingPipeline, CollisionPipeline>
{
public:
	using SharedPtr = std::shared_ptr<CollisionPipeline>;

	static SharedPtr create() { return SharedPtr(new CollisionPipeline()); }
	virtual ~CollisionPipeline() = default;

	virtual void onLoad(SampleCallbacks* pSample, const RenderContext::SharedPtr &pRenderContext) override;

protected:
	virtual void onFirstRun(SampleCallbacks* pSample) override;

	const std::string			mdefaultScenePath = "Data\\demo_scene\\demo_scene.fscene";
	RtScene::SharedPtr          rtScene = nullptr;
};