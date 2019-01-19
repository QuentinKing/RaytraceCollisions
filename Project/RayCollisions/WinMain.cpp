#include "Falcor.h"
#include "../SharedUtils/RenderingPipeline.h"
#include "Passes/GBufferPass.h"
#include "Passes/WriteToOutputPass.h"
#include "Passes/RaytraceGBufferPass.h"
#include "CollisionPipeline.h"

#define USE_RAYTRACING true

/* Main entry point of the program */
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	CollisionPipeline *pipeline = new CollisionPipeline();

	if (USE_RAYTRACING)
	{
		// Populate the raytracing rendering pipeline
		pipeline->setPass(0, RaytraceGBufferPass::create());
		pipeline->setPass(1, WriteToOutputPass::create());
	}
	else
	{
		// Populate our rasterization rendering pipeline
		pipeline->setPass(0, GBufferPass::create());
		pipeline->setPass(1, WriteToOutputPass::create());
	}

	// Configure the window for our program
	SampleConfig config;
	config.windowDesc.title = "Ray-based collision optimization testing";
	config.windowDesc.resizableWindow = true;

	// Run!
	CollisionPipeline::run(pipeline, config);
}