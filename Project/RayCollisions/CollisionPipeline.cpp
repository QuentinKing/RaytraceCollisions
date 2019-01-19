#include "CollisionPipeline.h"

void CollisionPipeline::onLoad(SampleCallbacks* pSample, const RenderContext::SharedPtr &pRenderContext)
{
	RenderingPipeline::onLoad(pSample, pRenderContext);

	mpResourceManager->setDefaultSceneName(mdefaultScenePath);
	updatePipelineRequirementFlags();
}