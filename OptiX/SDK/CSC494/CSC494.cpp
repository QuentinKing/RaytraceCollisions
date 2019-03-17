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
#include <vector>
#include <math.h>
#include <stdint.h>

// User created headers / includes
#include <sutil.h>
#include "random.h"
#include <Arcball.h>
#include "RigidBody.h"
#include "GeomteryCreator.h"
#include "Lights.h"

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
double		 last_update_time = 0;

std::string  texture_path;
const char*  scene_ptx;

// Geometry
std::vector<RigidBody> sceneRigidBodies;

// Camera setup
enum CameraType
{
	Perspective = 0,
	Orthographic = 1
};
const CameraType cameraType = CameraType::Orthographic;

// Buffer to render
enum RenderBuffer
{
	ShadedOutput = 0,
	IntersectionVolume = 1
};
RenderBuffer curRenderBuffer = RenderBuffer::ShadedOutput;

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

Buffer GetOutputBuffer();
Buffer GetVolumeVisualBuffer();
Buffer GetVolumeBuffer();

void DestroyContext();
void RegisterExitHandler();
void CreateContext();
void CreateScene();
void CreateLights();
void SetupCamera();
void UpdateGeometry();
void UpdateCamera();
void GlutInitialize(int* argc, char** argv);
void GlutRun();

void GlutDisplay();
void GlutKeyboardPress(unsigned char k, int x, int y);
void GlutMousePress(int button, int state, int x, int y);
void GlutMouseMotion(int x, int y);
void GlutResize(int w, int h);


//------------------------------------------------------------------------------
//
//  Helper functions
//
//------------------------------------------------------------------------------

Buffer GetOutputBuffer()
{
	return context["output_buffer"]->getBuffer();
}

Buffer GetVolumeVisualBuffer()
{
	return context["volume_visual_buffer"]->getBuffer();
}

Buffer GetVolumeBuffer()
{
	return context["volume_buffer"]->getBuffer();
}

void DestroyContext()
{
	if (context)
	{
		context->destroy();
		context = 0;
	}
}


void RegisterExitHandler()
{
	// register shutdown handler
#ifdef _WIN32
	glutCloseFunc(DestroyContext);  // this function is freeglut-only
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
	context["scene_epsilon"]->setFloat(1.e-4f);

	// Ray types
	context["radiance_ray_type"]->setUint( 0 );
    context["shadow_ray_type"]->setUint( 1 );

	last_update_time = sutil::currentTime();

	// Output buffers
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 4 * width * height, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	Buffer buffer = sutil::createOutputBuffer(context, RT_FORMAT_UNSIGNED_BYTE4, width, height, use_pbo);
	context["output_buffer"]->set(buffer);

	Buffer volume_visual_buffer = context->createBuffer(RT_BUFFER_INPUT_OUTPUT, RT_FORMAT_UNSIGNED_BYTE4, width, height);
	context["volume_visual_buffer"]->set(volume_visual_buffer);

	Buffer volume_buffer = context->createBuffer(RT_BUFFER_INPUT_OUTPUT, RT_FORMAT_FLOAT, width, height);
	context["volume_buffer"]->set(volume_buffer);

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

	// Miss program
	const std::string miss_name = "miss";
	context->setMissProgram(0, context->createProgramFromPTXString(scene_ptx, miss_name));
	context["bg_color"]->setFloat(make_float3(0.34f, 0.55f, 0.85f));

	// Exception program
	Program exception_program = context->createProgramFromPTXString(scene_ptx, "exception");
	context->setExceptionProgram(0, exception_program);
	context["bad_color"]->setFloat(1.0f, 0.0f, 0.80f);
}


void CreateScene()
{
	GeometryCreator geometryCreator(context, PROJECT_NAME, SCENE_NAME);

	// All non-rigidbody geometry
	std::vector<GeometryInstance> staticGeometry;
	GeometryGroup staticGroup = context->createGeometryGroup();

	// Create root scene group
	Group sceneGroup = context->createGroup();

	// Create floor (not a rigidbody)
	GeometryInstance planeInstance = geometryCreator.CreatePlane(make_float3(-64.0f, 0.0f, -64.0f),
		make_float3(128.0f, 0.0f, 0.0f),
		make_float3(0.0f, 0.0f, 128.0f));
	staticGeometry.push_back(planeInstance);

	// Create rigidbody spheres
	GeometryInstance sphereInstance = geometryCreator.CreateSphere(3.0f);
	RigidBody rigidBody(context, sphereInstance, make_float3(0.0f, 4.0, 4.0f), 1.0f, false);
	rigidBody.RegisterPlane(make_float3(-64.0f, 0.0f, -64.0f), make_float3(0.0f, 1.0f, 0.0f));
	rigidBody.AddForce(make_float3(0.0f, 0.0f, -8.0f));
	rigidBody.AddTorque(make_float3(0.0f, 0.0f, 0.0f));
	sceneRigidBodies.push_back(rigidBody);

	sphereInstance = geometryCreator.CreateSphere(3.0f);
	rigidBody = RigidBody(context, sphereInstance, make_float3(0.0f, 4.0, -4.0f), 1.0f, false);
	rigidBody.RegisterPlane(make_float3(-64.0f, 0.0f, -64.0f), make_float3(0.0f, 1.0f, 0.0f));
	rigidBody.AddForce(make_float3(0.0f, 0.0f, 8.0f));
	rigidBody.AddTorque(make_float3(0.0f, 0.0f, 0.0f));
	sceneRigidBodies.push_back(rigidBody);

	GeometryInstance boxInstance = geometryCreator.CreateBox(make_float3(3.0f, 3.0f, 3.0f));
	rigidBody = RigidBody(context, boxInstance, make_float3(6.0f, 4.0f, 0.0f), 1.0f, false);
	rigidBody.AddForce(make_float3(0.0f, 0.0f, 0.0f));
	rigidBody.AddTorque(make_float3(0.25f, 0.5f, 1.0f));
	//rigidBody.SetRotation(make_float4(0.0f, 0.0f, -0.3826834f, 0.9238795f));
	sceneRigidBodies.push_back(rigidBody);


	// Create static geometry group
	staticGroup->setChildCount(static_cast<unsigned int>(staticGeometry.size()));
	for (uint i = 0; i < staticGeometry.size(); i++)
	{
		staticGroup->setChild(i, staticGeometry[i]);
	}
	staticGroup->setAcceleration(context->createAcceleration("NoAccel"));

	// Set up scene group
	sceneGroup->setChildCount(sceneRigidBodies.size() + 1); // +1 for static geometry
	for (uint i = 0; i < sceneRigidBodies.size(); i++)
	{
		sceneGroup->setChild(i, sceneRigidBodies[i].GetTransform());
	}
	sceneGroup->setChild(sceneRigidBodies.size(), staticGroup);
	sceneGroup->setAcceleration(context->createAcceleration("NoAccel"));

	context["top_object"]->set(sceneGroup);
	context["top_shadower"]->set(sceneGroup);

	CreateLights();
}

void CreateLights()
{
	Light lights[] =
	{
        { make_float3( 0.0f, 20.0f, 0.0f ), make_float3( 1.0f, 1.0f, 1.0f ), 1 }
    };

    Buffer light_buffer = context->createBuffer( RT_BUFFER_INPUT );
    light_buffer->setFormat( RT_FORMAT_USER );
    light_buffer->setElementSize( sizeof( Light ) );
    light_buffer->setSize( sizeof(lights)/sizeof(lights[0]) );
    memcpy(light_buffer->map(), lights, sizeof(lights));
    light_buffer->unmap();

	context["ambientLightColor"]->setFloat( 0.31f, 0.33f, 0.28f );
    context["lights"]->set( light_buffer );
}

void SetupCamera()
{
	camera_eye = make_float3(7.0f, 9.2f, -6.0f);
	camera_lookat = make_float3(0.0f, 4.0f, 0.0f);
	camera_up = make_float3(0.0f, 1.0f, 0.0f);

	camera_rotate = Matrix4x4::identity();
}

void UpdateGeometry()
{
	float updateTime = sutil::currentTime() - last_update_time;
	float deltaTime = std::fmin(updateTime, 0.1f); // For numerical stability
	for (auto i = sceneRigidBodies.begin(); i != sceneRigidBodies.end(); ++i)
	{
		i->EulerStep(deltaTime);
	}

	last_update_time = sutil::currentTime();
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

	context["orthoCameraSize"]->setFloat(make_float2(10.0f, 10.0f * ((float)height/width)));
}


void GlutInitialize(int* argc, char** argv)
{
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow(PROJECT_NAME);
	glutHideWindow();
}


void GlutRun()
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
	glutDisplayFunc(GlutDisplay);
	glutIdleFunc(GlutDisplay);
	glutReshapeFunc(GlutResize);
	glutKeyboardFunc(GlutKeyboardPress);
	glutMouseFunc(GlutMousePress);
	glutMotionFunc(GlutMouseMotion);

	RegisterExitHandler();

	glutMainLoop();
}


//------------------------------------------------------------------------------
//
//  GLUT callbacks
//
//------------------------------------------------------------------------------

void GlutDisplay()
{
	UpdateGeometry();
	UpdateCamera();

	context->launch(0, width, height);

	Buffer renderBuffer = GetOutputBuffer();
	Buffer volumeVisualBuffer = GetVolumeVisualBuffer();
	Buffer volumeBuffer = GetVolumeBuffer();
	switch (curRenderBuffer)
	{
	case RenderBuffer::ShadedOutput:
		sutil::displayBufferGL(renderBuffer);
		break;
	case RenderBuffer::IntersectionVolume:
		sutil::displayBufferGL(volumeVisualBuffer);
		break;
	}

	// Report intersection volume
	float volume = 0.0f;
	RTsize width, height;
	volumeBuffer->getSize(width, height);
	void* data = volumeBuffer->map();
	float* volumeData = (float*)data;

	for(uint i = 0; i < width*height; i++)
	{
		volume += volumeData[i];
	}
	volumeBuffer->unmap();

	// Display intersection volume
	char volumeText[64];
	snprintf(volumeText, sizeof volumeText, "%f", volume);
	sutil::displayText(volumeText, 25, height-25);

	// Display collision detection
	if (volume > 0.0f)
	{
		char* collisionText = "Rigidbody collision detected!";
		sutil::displayText(collisionText, 25, height-55);
	}

	// Display frames per second
	sutil::displayFps(frame_count++);

	glutSwapBuffers();

	// Resolve collisions
}


void GlutKeyboardPress(unsigned char k, int x, int y)
{

	switch (k)
	{
		case('q'):
		case(27): // ESC
		{
			DestroyContext();
			exit(0);
		}
		case('s'):
		{
			const std::string outputImage = std::string(PROJECT_NAME) + ".ppm";
			std::cerr << "Saving current frame to '" << outputImage << "'\n";
			sutil::displayBufferPPM(outputImage.c_str(), GetOutputBuffer());
			break;
		}
		case('v'):
		{
			curRenderBuffer = RenderBuffer::ShadedOutput;
			break;
		}
		case('b'):
		{
			curRenderBuffer = RenderBuffer::IntersectionVolume;
			break;
		}
	}
}


void GlutMousePress(int button, int state, int x, int y)
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


void GlutMouseMotion(int x, int y)
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


void GlutResize(int w, int h)
{
	if (w == (int)width && h == (int)height) return;

	width = w;
	height = h;
	sutil::ensureMinimumSize(width, height);

	sutil::resizeBuffer(GetOutputBuffer(), width, height);

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
		GlutInitialize(&argc, argv);

#ifndef __APPLE__
		glewInit();
#endif

		// Load PTX source
		scene_ptx = sutil::getPtxString(PROJECT_NAME, SCENE_NAME);

		CreateContext();
		CreateScene();
		SetupCamera();

		context->validate();

		if (out_file.empty())
		{
			GlutRun();
		}
		else
		{
			UpdateCamera();
			context->launch(0, width, height);
			sutil::displayBufferPPM(out_file.c_str(), GetOutputBuffer());
			DestroyContext();
		}
		return 0;
	}
	SUTIL_CATCH(context->get())
}
