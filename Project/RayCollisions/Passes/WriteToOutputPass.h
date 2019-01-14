#pragma once
#include "../SharedUtils/RenderPass.h"

class WriteToOutputPass : public ::RenderPass, inherit_shared_from_this<::RenderPass, WriteToOutputPass>
{
public:
	using SharedPtr = std::shared_ptr<WriteToOutputPass>;

	static SharedPtr create() { return SharedPtr(new WriteToOutputPass()); }
	virtual ~WriteToOutputPass() = default;

protected:
	WriteToOutputPass() : ::RenderPass("Output Pass", "Output Pass Options") {}

	bool initialize(RenderContext* pRenderContext, ResourceManager::SharedPtr pResManager) override;
	void renderGui(Gui* pGui) override;
	void pipelineUpdated(ResourceManager::SharedPtr pResManager) override; // Called whenever the pipeline sequence changes
	void execute(RenderContext* pRenderContext) override;

	bool appliesPostprocess() override { return true; }

	Gui::DropdownList mDisplayableBuffers;
	uint32_t          mSelectedBuffer = 0xFFFFFFFFu;
};
