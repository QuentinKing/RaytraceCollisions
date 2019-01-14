#include "WriteToOutputPass.h"

bool WriteToOutputPass::initialize(RenderContext* pRenderContext, ResourceManager::SharedPtr pResManager)
{
	mpResManager = pResManager;
	mpResManager->requestTextureResource(ResourceManager::kOutputChannel); // Writing to the output

	mDisplayableBuffers.push_back({ -1, "< None >" }); // GUI dropdown
	return true;
}

void WriteToOutputPass::renderGui(Gui* pGui)
{
	pGui->addDropdown("Displayed", mDisplayableBuffers, mSelectedBuffer); // Select buffer to render
}

void WriteToOutputPass::execute(RenderContext* pRenderContext)
{
	Texture::SharedPtr outTex = mpResManager->getTexture(ResourceManager::kOutputChannel);
	if (!outTex) return;

	Texture::SharedPtr inTex = mpResManager->getTexture(mSelectedBuffer);

	// Invalid buffer
	if (!inTex || mSelectedBuffer == uint32_t(-1))
	{
		pRenderContext->clearRtv(outTex->getRTV().get(), vec4(0.0f, 0.0f, 0.0f, 1.0f));
		return;
	}

	pRenderContext->blit(inTex->getSRV(), outTex->getRTV());
}


void WriteToOutputPass::pipelineUpdated(ResourceManager::SharedPtr pResManager)
{
	if (!pResManager) return;

	if (pResManager != mpResManager)
		mpResManager = pResManager;

	// Recalculate what buffers we are able to display, since they were just updated
	mDisplayableBuffers.clear();

	int32_t outputChannel = mpResManager->getTextureIndex(ResourceManager::kOutputChannel);
	for (uint32_t i = 0; i < mpResManager->getTextureCount(); i++)
	{
		if (i == outputChannel) continue;

		mDisplayableBuffers.push_back({ int32_t(i), mpResManager->getTextureName(i) });
		if (mSelectedBuffer == uint32_t(-1)) mSelectedBuffer = i; // If we currently have an invalid buffer, switch to this one
	}

	// No valid textures
	if (mDisplayableBuffers.size() <= 0)
	{
		mDisplayableBuffers.push_back({ -1, "< None >" });
		mSelectedBuffer = uint32_t(-1);
	}
}