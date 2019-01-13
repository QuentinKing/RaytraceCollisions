#pragma once

#include "../SharedUtils/RenderPass.h" // This is the header including the base RenderPass class
#include "../SharedUtils/FullscreenLaunch.h"

class ConstantColorPass : public ::RenderPass, inherit_shared_from_this<::RenderPass, ConstantColorPass>
{
public:
    using SharedPtr = std::shared_ptr<ConstantColorPass>;

	// Our simple constructor and destructor
	static SharedPtr create() { return SharedPtr(new ConstantColorPass()); }
    virtual ~ConstantColorPass() = default;

protected:
	// Constructor.  The strings represent:
	//     1) The name of the pass that will be in the dropdown pass selector widget(s)
	//     2) The name of the GUI window showing widget controls for this pass
	ConstantColorPass() : ::RenderPass("Custom Render Pass", "Custom Pass Options") {}
	
	// The initialize() callback will be invoked when this class is instantiated and bound to a pipeline
    bool initialize(RenderContext* pRenderContext, ResourceManager::SharedPtr pResManager) override;

	// The renderGui() callback allows you to attach GUI widget into this pass' options window
    void renderGui(Gui* pGui) override;

	// The execute() callback is invoked during frame render when it is this pass' turn to execute
    void execute(RenderContext* pRenderContext) override;

	// Override default RenderPass functionality (that control the rendering pipeline and its GUI)
	bool usesRasterization() override { return true; } 
	bool hasAnimation() override { return false; }  // Removes a GUI control that is confusing for this simple demo

	// Internal state variables for this pass
	vec3						  mConstColor = vec3(0.8f, 0.4f, 0.4f);		  // The color we'll use to for drawing
	FullscreenLaunch::SharedPtr   mpCustomPass;							      // Our accumulation shader state
	GraphicsState::SharedPtr      mpGfxState;								  // Our graphics pipeline state
	uint32_t                      mFrameCount = 0;							  // A frame counter to let our sinusoid animate
	float                         mScaleValue = 0.1f;						  // A scale value for our sinusoid
 };
