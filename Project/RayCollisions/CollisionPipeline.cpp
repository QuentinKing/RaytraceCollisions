#include "CollisionPipeline.h"

void CollisionPipeline::onLoad(SampleCallbacks* pSample, const RenderContext::SharedPtr &pRenderContext)
{
	RenderingPipeline::onLoad(pSample, pRenderContext);

	mpResourceManager->setDefaultSceneName(mdefaultScenePath);
	updatePipelineRequirementFlags();
}

void CollisionPipeline::onFirstRun(SampleCallbacks* pSample)
{
	RenderingPipeline::onFirstRun(pSample);

	rtScene = std::dynamic_pointer_cast<RtScene>(mpScene);
}