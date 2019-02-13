// OpenGL
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/freeglut.h>

// OptiX
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

// STL
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <math.h>
#include <stdint.h>

// User created headers / includes
#include <sutil.h>
#include "commonStructs.h"
#include "random.h"
#include <Arcball.h>
#include "GeomteryCreator.h"

using namespace optix;

//------------------------------------------------------------------------------
//
// Globals
//
//------------------------------------------------------------------------------

const char* const PROJECT_NAME = "CSC494";
const char* const SCENE_NAME = "ray_scene.cu";

Context      context;
uint32_t     width = 1080u;
uint32_t     height = 720;
bool         use_pbo = true;
unsigned	 frame_count = 0;

std::string  texture_path;
const char*  scene_ptx;

// Geometry state
GeometryInstance movingSphere;
GeometryInstance movingSphere2;

// Camera setup
enum CameraType
{
	Perspective = 0,
	Orthographic = 1
};
const CameraType cameraType = CameraType::Orthographic;

// Camera state
float3       camera_up;
float3       camera_lookat;
float3       camera_eye;
Matrix4x4    camera_rotate;
sutil::Arcball arcball;

// Mouse state
int2       mouse_prev_pos;
int        mouse_button;


//------------------------------------------------------------------------------
//
// Forward declarations
//
//------------------------------------------------------------------------------

Buffer getOutputBuffer();
void destroyContext();
void registerExitHandler();
void CreateContext();
void CreateScene();
void SetupCamera();
void SetupLights();
void UpdateCamera();
void glutInitialize(int* argc, char** argv);
void glutRun();

void glutDisplay();
void glutKeyboardPress(unsigned char k, int x, int y);
void glutMousePress(int button, int state, int x, int y);
void glutMouseMotion(int x, int y);
void glutResize(int w, int h);


//------------------------------------------------------------------------------
//
//  Helper functions
//
//------------------------------------------------------------------------------

Buffer getOutputBuffer()
{
	return context["output_buffer"]->getBuffer();
}


void destroyContext()
{
	if (context)
	{
		context->destroy();
		context = 0;
	}
}


void registerExitHandler()
{
	// register shutdown handler
#ifdef _WIN32
	glutCloseFunc(destroyContext);  // this function is freeglut-only
#else
	atexit(destroyContext);
#endif
}


void CreateContext()
{
	// Set up context
	context = Context::create();
	context->setRayTypeCount(2);	// The number of types of rays (shading, shadowing)
	context->setEntryPointCount(1); // Entry points, one for each ray generation algorithm (used for multipass rendering)
	context->setStackSize(4640);	// Allocated stack for each thread of execution

	// Note: high max depth for reflection and refraction through glass
	context["max_depth"]->setInt(100);
	context["radiance_ray_type"]->setUint(0);
	context["shadow_ray_type"]->setUint(1);
	context["scene_epsilon"]->setFloat(1.e-4f);
	context["importance_cutoff"]->setFloat(0.01f);
	context["ambient_light_color"]->setFloat(0.31f, 0.33f, 0.28f);

	// Output buffer
	// First allocate the memory for the GL buffer, then attach it to OptiX.
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 4 * width * height, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	Buffer buffer = sutil::createOutputBuffer(context, RT_FORMAT_UNSIGNED_BYTE4, width, height, use_pbo);
	context["output_buffer"]->set(buffer);

	// Determine what type of camera we are using
	std::string camera_name;
	switch (cameraType)
	{
	case CameraType::Perspective :
		camera_name = "perspective_camera";
		break;
	case CameraType::Orthographic :
		camera_name = "orthographic_camera";
		break;
	}

	// Ray generation program
	Program ray_gen_program = context->createProgramFromPTXString(scene_ptx, camera_name);
	context->setRayGenerationProgram(0, ray_gen_program);

	// Exception program
	Program exception_program = context->createProgramFromPTXString(scene_ptx, "exception");
	context->setExceptionProgram(0, exception_program);
	context["bad_color"]->setFloat(1.0f, 0.0f, 1.0f);

	// Miss program
	const std::string miss_name = "miss";
	context->setMissProgram(0, context->createProgramFromPTXString(scene_ptx, miss_name));
	const float3 default_color = make_float3(1.0f, 1.0f, 1.0f);
	const std::string texpath = texture_path + "/" + std::string("CedarCity.hdr");
	context["envmap"]->setTextureSampler(sutil::loadTexture(context, texpath, default_color));
	context["bg_color"]->setFloat(make_float3(0.34f, 0.55f, 0.85f));

	// 3D solid noise buffer, 1 float channel, all entries in the range [0.0, 1.0].

	const int tex_width = 64;
	const int tex_height = 64;
	const int tex_depth = 64;
	Buffer noiseBuffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, tex_width, tex_height, tex_depth);
	float *tex_data = (float *)noiseBuffer->map();

	// Random noise in range [0, 1]
	for (int i = tex_width * tex_height * tex_depth; i > 0; i--) {
		// One channel 3D noise in [0.0, 1.0] range.
		static unsigned int seed = 0u;
		*tex_data++ = rnd(seed);
	}
	noiseBuffer->unmap();


	// Noise texture sampler
	TextureSampler noiseSampler = context->createTextureSampler();

	noiseSampler->setWrapMode(0, RT_WRAP_REPEAT);
	noiseSampler->setWrapMode(1, RT_WRAP_REPEAT);
	noiseSampler->setFilteringModes(RT_FILTER_LINEAR, RT_FILTER_LINEAR, RT_FILTER_NONE);
	noiseSampler->setIndexingMode(RT_TEXTURE_INDEX_NORMALIZED_COORDINATES);
	noiseSampler->setReadMode(RT_TEXTURE_READ_NORMALIZED_FLOAT);
	noiseSampler->setMaxAnisotropy(1.0f);
	noiseSampler->setMipLevelCount(1);
	noiseSampler->setArraySize(1);
	noiseSampler->setBuffer(0, 0, noiseBuffer);

	context["noise_texture"]->setTextureSampler(noiseSampler);
}


void CreateScene()
{
	GeometryCreator geometryCreator(context, PROJECT_NAME, SCENE_NAME);

	GeometryInstance sphereInstance = geometryCreator.CreateSphere(make_float3(0,0,0), 3.0f);
	GeometryInstance sphereInstance2 = geometryCreator.CreateSphere(make_float3(0, 0, 0), 3.0f);
	//GeometryInstance boxInstance = geometryCreator.CreateBox(make_float3(-2.0f, 0.0f, -2.0f), make_float3(2.0f, 7.0f, 2.0f));
	GeometryInstance planeInstance = geometryCreator.CreatePlane(make_float3(-64.0f, 0.01f, -64.0f),
		make_float3(128.0f, 0.0f, 0.0f),
		make_float3(0.0f, 0.0f, 128.0f));

	movingSphere = sphereInstance;
	movingSphere2 = sphereInstance2;

	// Create GIs for each piece of geometry
	// Geomtery Instance -> coupling geometry and materials together
	std::vector<GeometryInstance> gis;
	gis.push_back(sphereInstance);
	gis.push_back(sphereInstance2);
	//gis.push_back(boxInstance);
	gis.push_back(planeInstance);

	// Geometry group -> coupling some number of instances with an acceleration structure
	GeometryGroup geometrygroup = context->createGeometryGroup();
	geometrygroup->setChildCount(static_cast<unsigned int>(gis.size()));
	for (uint i = 0; i < gis.size(); i++)
	{
		geometrygroup->setChild(i, gis[i]);
	}
	geometrygroup->setAcceleration(context->createAcceleration("NoAccel"));

	context["top_object"]->set(geometrygroup);
}


void SetupCamera()
{
	camera_eye = make_float3(7.0f, 9.2f, -6.0f);
	camera_lookat = make_float3(0.0f, 4.0f, 0.0f);
	camera_up = make_float3(0.0f, 1.0f, 0.0f);

	camera_rotate = Matrix4x4::identity();
}


void SetupLights()
{

	BasicLight lights[] = {
		{ make_float3(-5.0f, 60.0f, -16.0f), make_float3(1.0f, 1.0f, 1.0f), 1 }
	};

	Buffer light_buffer = context->createBuffer(RT_BUFFER_INPUT);
	light_buffer->setFormat(RT_FORMAT_USER);
	light_buffer->setElementSize(sizeof(BasicLight));
	light_buffer->setSize(sizeof(lights) / sizeof(lights[0]));
	memcpy(light_buffer->map(), lights, sizeof(lights));
	light_buffer->unmap();

	context["lights"]->set(light_buffer);
}

void UpdateGeometry()
{
	if (movingSphere)
	{
		// Bounce sphere around
		double yMovement = sin(frame_count / 60.0f) * 3.0f;
		double xMovement = sin(frame_count / 70.0f) * 4.0f;
		double zMovement = sin(frame_count / 80.0f) * 2.0f;
		float4 sphereData = make_float4(xMovement, 7.0f + yMovement, zMovement, 3.0f);
		movingSphere["sphere"]->setFloat(sphereData);
	}

	if (movingSphere2)
	{
		// Bounce sphere around
		double yMovement = -sin(frame_count / 60.0f) * 3.0f;
		double xMovement = -sin(frame_count / 70.0f) * 4.0f;
		double zMovement = -sin(frame_count / 80.0f) * 2.0f;
		float4 sphereData = make_float4(xMovement, 7.0f + yMovement, zMovement, 3.0f);
		movingSphere2["sphere"]->setFloat(sphereData);
	}
}

void UpdateCamera()
{
	const float vfov = 60.0f;
	const float aspect_ratio = static_cast<float>(width) /
		static_cast<float>(height);

	float3 camera_u, camera_v, camera_w;
	sutil::calculateCameraVariables(
		camera_eye, camera_lookat, camera_up, vfov, aspect_ratio,
		camera_u, camera_v, camera_w, true);

	const Matrix4x4 frame = Matrix4x4::fromBasis(
		normalize(camera_u),
		normalize(camera_v),
		normalize(-camera_w),
		camera_lookat);
	const Matrix4x4 frame_inv = frame.inverse();
	// Apply camera rotation twice to match old SDK behavior
	const Matrix4x4 trans = frame * camera_rotate*camera_rotate*frame_inv;

	camera_eye = make_float3(trans*make_float4(camera_eye, 1.0f));
	camera_lookat = make_float3(trans*make_float4(camera_lookat, 1.0f));
	camera_up = make_float3(trans*make_float4(camera_up, 0.0f));

	sutil::calculateCameraVariables(
		camera_eye, camera_lookat, camera_up, vfov, aspect_ratio,
		camera_u, camera_v, camera_w, true);

	camera_rotate = Matrix4x4::identity();

	context["eye"]->setFloat(camera_eye);
	context["U"]->setFloat(camera_u);
	context["V"]->setFloat(camera_v);
	context["W"]->setFloat(camera_w);

	context["orthoCameraSize"]->setFloat(make_float2(1.0f, 1.0f));
}


void glutInitialize(int* argc, char** argv)
{
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow(PROJECT_NAME);
	glutHideWindow();
}


void glutRun()
{
	// Initialize GL state
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glViewport(0, 0, width, height);

	glutShowWindow();
	glutReshapeWindow(width, height);

	// GLUT callbacks
	glutDisplayFunc(glutDisplay);
	glutIdleFunc(glutDisplay);
	glutReshapeFunc(glutResize);
	glutKeyboardFunc(glutKeyboardPress);
	glutMouseFunc(glutMousePress);
	glutMotionFunc(glutMouseMotion);

	registerExitHandler();

	glutMainLoop();
}


//------------------------------------------------------------------------------
//
//  GLUT callbacks
//
//------------------------------------------------------------------------------

void glutDisplay()
{
	UpdateGeometry();
	UpdateCamera();

	context->launch(0, width, height);

	Buffer buffer = getOutputBuffer();
	sutil::displayBufferGL(getOutputBuffer());

	sutil::displayFps(frame_count++);

	glutSwapBuffers();
}


void glutKeyboardPress(unsigned char k, int x, int y)
{

	switch (k)
	{
		case('q'):
		case(27): // ESC
		{
			destroyContext();
			exit(0);
		}
		case('s'):
		{
			const std::string outputImage = std::string(PROJECT_NAME) + ".ppm";
			std::cerr << "Saving current frame to '" << outputImage << "'\n";
			sutil::displayBufferPPM(outputImage.c_str(), getOutputBuffer());
			break;
		}
	}
}


void glutMousePress(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		mouse_button = button;
		mouse_prev_pos = make_int2(x, y);
	}
	else
	{
		// nothing
	}
}


void glutMouseMotion(int x, int y)
{
	if (mouse_button == GLUT_RIGHT_BUTTON)
	{
		const float dx = static_cast<float>(x - mouse_prev_pos.x) /
			static_cast<float>(width);
		const float dy = static_cast<float>(y - mouse_prev_pos.y) /
			static_cast<float>(height);
		const float dmax = fabsf(dx) > fabs(dy) ? dx : dy;
		const float scale = fminf(dmax, 0.9f);
		camera_eye = camera_eye + (camera_lookat - camera_eye)*scale;
	}
	else if (mouse_button == GLUT_LEFT_BUTTON)
	{
		const float2 from = { static_cast<float>(mouse_prev_pos.x),
							  static_cast<float>(mouse_prev_pos.y) };
		const float2 to = { static_cast<float>(x),
							  static_cast<float>(y) };

		const float2 a = { from.x / width, from.y / height };
		const float2 b = { to.x / width, to.y / height };

		camera_rotate = arcball.rotate(b, a);
	}

	mouse_prev_pos = make_int2(x, y);
}


void glutResize(int w, int h)
{
	if (w == (int)width && h == (int)height) return;

	width = w;
	height = h;
	sutil::ensureMinimumSize(width, height);

	sutil::resizeBuffer(getOutputBuffer(), width, height);

	glViewport(0, 0, width, height);

	glutPostRedisplay();
}


//------------------------------------------------------------------------------
//
// Main
//
//------------------------------------------------------------------------------

void printUsageAndExit(const std::string& argv0)
{
	std::cerr << "\nUsage: " << argv0 << " [options]\n";
	std::cerr <<
		"App Options:\n"
		"  -h | --help         Print this usage message and exit.\n"
		"  -f | --file         Save single frame to file and exit.\n"
		"  -n | --nopbo        Disable GL interop for display buffer.\n"
		"  -T | --tutorial-number <num>              Specify tutorial number\n"
		"  -t | --texture-path <path>                Specify path to texture directory\n"
		"App Keystrokes:\n"
		"  q  Quit\n"
		"  s  Save image to '" << PROJECT_NAME << ".ppm'\n"
		<< std::endl;

	exit(1);
}

int main(int argc, char** argv)
{
	std::string out_file;
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
		else if (arg == "-t" || arg == "--texture-path")
		{
			if (i == argc - 1)
			{
				std::cerr << "Option '" << arg << "' requires additional argument.\n";
				printUsageAndExit(argv[0]);
			}
			texture_path = argv[++i];
		}
		else
		{
			std::cerr << "Unknown option '" << arg << "'\n";
			printUsageAndExit(argv[0]);
		}
	}

	if (texture_path.empty())
	{
		texture_path = std::string(sutil::samplesDir()) + "/data";
	}

	try
	{
		glutInitialize(&argc, argv);

#ifndef __APPLE__
		glewInit();
#endif

		// Load PTX source
		scene_ptx = sutil::getPtxString(PROJECT_NAME, SCENE_NAME);

		CreateContext();
		CreateScene();
		SetupCamera();
		SetupLights();

		context->validate();

		if (out_file.empty())
		{
			glutRun();
		}
		else
		{
			UpdateCamera();
			context->launch(0, width, height);
			sutil::displayBufferPPM(out_file.c_str(), getOutputBuffer());
			destroyContext();
		}
		return 0;
	}
	SUTIL_CATCH(context->get())
}
