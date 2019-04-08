#include "Scene.h"

using namespace optix;

const char* const PROJECT_NAME = "CSC494";

void printUsageAndExit(const std::string& argv0)
{
	std::cerr << "\nUsage: " << argv0 << " [options]\n";
	std::cerr <<
		"App Options:\n"
		"  -h | --help         Print this usage message and exit.\n"
		"  -f | --file         Save single frame to file and exit.\n"
		"  -n | --nopbo        Disable GL interop for display buffer.\n"
		"App Keystrokes:\n"
		"  q  Quit\n"
		"  s  Save image to '" << PROJECT_NAME << ".ppm'\n"
		<< std::endl;

	exit(1);
}

int main(int argc, char** argv)
{
	std::string out_file;
	bool use_pbo = true;
	for (int i = 1; i < argc; ++i)
	{
		const std::string arg(argv[i]);

		if (arg == "-h" || arg == "--help")
		{
			printUsageAndExit(argv[0]);
		}
		else if (arg == "-f" || arg == "--file")
		{
			if (i == argc - 1)
			{
				std::cerr << "Option '" << arg << "' requires additional argument.\n";
				printUsageAndExit(argv[0]);
			}
			out_file = argv[++i];
		}
		else if (arg == "-n" || arg == "--nopbo")
		{
			use_pbo = false;
		}
		else
		{
			std::cerr << "Unknown option '" << arg << "'\n";
			printUsageAndExit(argv[0]);
		}
	}

	Scene::Get().Setup(argc, argv, out_file, use_pbo);
}
