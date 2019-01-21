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

	// Add in a bunch of dummy instances for later
	if (rtScene->getModelCount() > 0 && rtScene->getModelInstanceCount(0) > 0)
	{
		createTempInstances(0);

		// Move in some instaces to play around with
		const float r = 5.0f;
		for (int i = 0; i < 10; i++)
		{
			const auto& pInstance = rtScene->getModelInstance(0, i);
			pInstance->setTranslation(glm::vec3(sin(i * 0.62831853071) * r, cos(i * 0.62831853071) * r, 0.0), false);
		}
	}

	// When a new scene is loaded, we'll tell all our passes about it (not just active passes)
	for (uint32_t i = 0; i < mAvailPasses.size(); i++)
	{
		if (mAvailPasses[i])
		{
			mAvailPasses[i]->onInitScene(pRenderContext, rtScene);
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

void CollisionPipeline::createTempInstances(uint32 modelIndex)
{
	// Believe that somewhere they enforce maximum of 64 instances
	for (int i = 1; i < 64; i++)
	{
		const auto& pInstance = rtScene->getModelInstance(modelIndex, 0);
		auto& pModel = rtScene->getModel(modelIndex);
		std::string name = "Instance" + std::to_string(i);
		rtScene->addModelInstance(pModel, name, glm::vec3(-9999, -9999, -9999), pInstance->getRotation(), pInstance->getScaling());
	}
	rtScene->getModelInstance(modelIndex, 0)->setTranslation(glm::vec3(-9999, -9999, -9999), false);
}