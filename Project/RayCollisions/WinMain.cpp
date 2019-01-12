#include "Falcor.h"
#include "../SharedUtils/RenderingPipeline.h"
#include "Passes/ConstantColorPass.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Create our rendering pipeline
	RenderingPipeline *pipeline = new RenderingPipeline();

	// Add passes into our pipeline
	pipeline->setPass(0, ConstantColorPass::create());   // Displays a user-selectable color on the screen

	// Define a set of config / window parameters for our program
	SampleConfig config;
	config.windowDesc.title = "Ray-based collision optimization testing";
	config.windowDesc.resizableWindow = true;

	// Start our program!
	RenderingPipeline::run(pipeline, config);
}