#include "CollisionPipeline.h"
#include "../SharedUtils/SceneLoaderWrapper.h"

void CollisionPipeline::onLoad(SampleCallbacks* pSample, const RenderContext::SharedPtr &pRenderContext)
{
	RenderingPipeline::onLoad(pSample, pRenderContext);

	mpResourceManager->setDefaultSceneName(mdefaultScenePath);
	updatePipelineRequirementFlags();
}

void CollisionPipeline::onInitNewScene(RenderContext* pRenderContext, Scene::SharedPtr pScene)
{
	// Stash the scene in the pipeline
	if (pScene)
		mpScene = pScene;

	// Try to add the instance now
	const auto& pInstance = rtScene->getModelInstance(0, 0);
	auto& pModel = rtScene->getModel(0);
	std::string name = "SphereInstance";
	rtScene->addModelInstance(pModel, name, pInstance->getTranslation() + glm::vec3(1, 1, 1), pInstance->getRotation(), pInstance->getScaling());

	// When a new scene is loaded, we'll tell all our passes about it (not just active passes)
	for (uint32_t i = 0; i < mAvailPasses.size(); i++)
	{
		if (mAvailPasses[i])
		{
			mAvailPasses[i]->onInitScene(pRenderContext, pScene);
		}
	}
}

void CollisionPipeline::onFirstRun(SampleCallbacks* pSample)
{
	// Did the user ask for us to load a scene by default?
	if (mPipeNeedsDefaultScene)
	{
		rtScene = loadScene(mLastKnownSize, mpResourceManager->getDefaultSceneName().c_str());
		if (rtScene) onInitNewScene(pSample->getRenderContext().get(), rtScene);
	}

	mFirstFrame = false;
}