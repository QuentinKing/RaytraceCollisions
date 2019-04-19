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
#include <Arcball.h>
#include "RigidBody.h"
#include "GeometryCreator.h"
#include "BufferStructs.h"
#include "MathHelpers.h"
#include "Scene.h"

using namespace optix;

/*
 * Globals
 */
const char* const PROJECT_NAME = "CSC494";
const char* const SCENE_NAME = "ray_scene.cu";

Context      context;
uint32_t     width = 1080u;
uint32_t     height = 720u;
bool         use_pbo = true;
unsigned	 frame_count = 0;
double		 last_update_time = 0;

// This controls how many rays are used for volume detection.
// The higher the number, the lower the resolution is for the collision buffer but
// the performance of the program will increase
uint32_t	 physicsRayStep = 8;

const char*  scene_ptx;

// Geometry
std::vector<RigidBody> sceneRigidBodies;

// Camera state
float3       camera_up;
float3       camera_lookat;
float3       camera_eye;
Matrix4x4    camera_rotate;
sutil::Arcball arcball;

// Mouse state
int2       mouse_prev_pos;
int        mouse_button;

Buffer Scene::GetOutputBuffer()
{
	return context["output_buffer"]->getBuffer();
}

Buffer Scene::GetResponseBuffer()
{
	return context["collisionResponse"]->getBuffer();
}

void Scene::Setup(int argc, char** argv, std::string out_file, bool use_pbo)
{
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
	}
	SUTIL_CATCH(context->get())
}

void Scene::CreateContext()
{
	context = Context::create();
	context->setRayTypeCount(2);				// The number of types of rays (shading, shadowing)
	context->setEntryPointCount(1);				// Entry points, one for each ray generation algorithm (used for multipass rendering)
	context->setStackSize(4640);				// Allocated stack for each thread of execution

	context["scene_epsilon"]->setFloat(1.e-4f); // Min distance to check along the ray
	context["radiance_ray_type"]->setUint(0);	// Index of the radiance ray
    context["shadow_ray_type"]->setUint(1);		// Index of the shadow ray

	last_update_time = sutil::currentTime();	// Initialize time

	// Output buffers
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 4 * width * height, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	Buffer buffer = sutil::createOutputBuffer(context, RT_FORMAT_UNSIGNED_BYTE4, width, height, use_pbo);
	context["output_buffer"]->set(buffer);

	// Ray generation program
	Program ray_gen_program = context->createProgramFromPTXString(scene_ptx, "perspective_camera");
	context->setRayGenerationProgram(0, ray_gen_program);

	// Set scene ray variables
	context["importance_cutoff"]->setFloat(0.01f);
	context["max_depth"]->setInt(100);

	// Miss program
	context->setMissProgram(0, context->createProgramFromPTXString(scene_ptx, "miss"));
	const std::string texpath = std::string(sutil::samplesDir()) + "/data/Rathaus.hdr";
    context["envmap"]->setTextureSampler(sutil::loadTexture(context, texpath, make_float3(1.0, 1.0, 1.0)));

	// Exception program
	Program exception_program = context->createProgramFromPTXString(scene_ptx, "exception");
	context->setExceptionProgram(0, exception_program);
	context["bad_color"]->setFloat(0.0f, 1.0f, 0.0f);

	float importance_cutoff = 0.01;
	int max_depth = 100;
}

void Scene::DestroyContext()
{
	Scene instance = Scene::Get();

	if (instance.context)
	{
		instance.context->destroy();
		instance.context = 0;
	}
}

void Scene::CreateScene()
{
	GeometryCreator geometryCreator(context, PROJECT_NAME, SCENE_NAME);

	// Create root scene group
	Group sceneGroup = context->createGroup();

	MaterialProperties mat1 = MaterialProperties("closest_hit_radiance", make_float3(0.0f, 0.0f, 0.0f), make_float3(0.3f, 0.3f, 0.3f), make_float3(0.9f, 0.9f, 0.9f), 300.0f, make_float3(0.2f, 0.2f, 0.2f), make_float3(0.9f, 0.9f, 0.9f));
	MaterialProperties mat2 = MaterialProperties("closest_hit_radiance", make_float3(0.1f, 0.1f, 0.1f), make_float3(0.8f, 0.2f, 0.8f), make_float3(0.8f, 0.9f, 0.8f), 88.0f, make_float3(0.2f, 0.2f, 0.2f), make_float3(0.1f, 0.1f, 0.1f));
	MaterialProperties mat3 = MaterialProperties("closest_hit_radiance", make_float3(0.1f, 0.1f, 0.1f), make_float3(0.3f, 0.7f, 0.5f), make_float3(0.9f, 0.9f, 0.9f), 88.0f, make_float3(0.0f, 0.0f, 0.0f), make_float3(0.0f, 0.0f, 0.0f));

	// Create rigidbodies
	GeometryInstance sphereInstance = geometryCreator.CreateSphere(3.0f, mat1);
	RigidBody rigidBody(context, PROJECT_NAME, SCENE_NAME, sphereInstance, 0, make_float3(0.0f, 4.0, 15.0f), 1.0f, "NoAccel", false, false);
	rigidBody.AddForce(make_float3(0.0f, 0.0f, -150.0f));
	sceneRigidBodies.push_back(rigidBody);

	GeometryInstance boxInstance = geometryCreator.CreateBox(make_float3(3.0f, 3.0f, 3.0f), mat2);
	rigidBody = RigidBody(context, PROJECT_NAME, SCENE_NAME, boxInstance, 1, make_float3(0.5f, 6.0f, -15.0f), 1.0f, "NoAccel", false, false);
	rigidBody.AddForce(make_float3(0.0f, 0.0f, 150.0f));
	sceneRigidBodies.push_back(rigidBody);

	//GeometryInstance box2Instance = geometryCreator.CreateBox(make_float3(3.0f, 3.0f, 3.0f), mat2);
	//rigidBody = RigidBody(context, PROJECT_NAME, SCENE_NAME, box2Instance, 2, make_float3(-5.5f, 6.0f, 0.0f), 1.0f, "NoAccel", false, false);
	//rigidBody.AddForce(make_float3(55.0f, 0.0f, 0.0f));
	//sceneRigidBodies.push_back(rigidBody);

	//GeometryInstance mesh = geometryCreator.CreateMesh(std::string(sutil::samplesDir()) + "data/cow.obj", mat3);
	//rigidBody = RigidBody(context, PROJECT_NAME, SCENE_NAME, mesh, 1, make_float3(1.5f, 2.0f, -4.0f), 1.0f, "Trbvh", false, false);
	//rigidBody.AddForce(make_float3(0.0f, 0.0f, 50.0f));
	//sceneRigidBodies.push_back(rigidBody);

	// Set up scene group
	sceneGroup->setChildCount(sceneRigidBodies.size());
	for (uint i = 0; i < sceneRigidBodies.size(); i++)
	{
		sceneGroup->setChild(i, sceneRigidBodies[i].GetTransform());
	}
	sceneGroup->setAcceleration(context->createAcceleration("NoAccel"));

	context["top_object"]->set(sceneGroup);
	context["top_shadower"]->set(sceneGroup);

	CreateLights();

	// Create collision response buffer
	// Each pixel stores the volume of intersection between the pair of rigidbodies
	Buffer response_buffer = context->createBuffer( RT_BUFFER_INPUT_OUTPUT );
    response_buffer->setFormat( RT_FORMAT_USER );
    response_buffer->setElementSize( sizeof( IntersectionResponse ) );

	uint32_t physicsBufferWidth = width / physicsRayStep;
	uint32_t physicsBufferHeight = height / physicsRayStep;
    response_buffer->setSize(physicsBufferWidth, physicsBufferHeight);

	context["physicsRayStep"]->setInt(physicsRayStep);
	context["physicsBufferWidth"]->setInt(physicsBufferWidth);
	context["physicsBufferHeight"]->setInt(physicsBufferHeight);
	context["collisionResponse"]->set(response_buffer);
}

void Scene::CreateLights()
{
	Light lights[] =
	{
        { make_float3( 20.0f, 20.0f, 0.0f ), make_float3( 1.0f, 1.0f, 1.0f ), 1 }
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

void Scene::SetupCamera()
{
	camera_eye = make_float3(7.0f, 9.2f, -6.0f) * 10.0;
	camera_lookat = make_float3(0.0f, 4.0f, 0.0f);
	camera_up = make_float3(0.0f, 1.0f, 0.0f);

	camera_rotate = Matrix4x4::identity();
}

void Scene::UpdateGeometry()
{
	float updateTime = sutil::currentTime() - last_update_time;
	float deltaTime = std::fmin(updateTime, 0.1f); // For numerical stability
	for (auto i = sceneRigidBodies.begin(); i != sceneRigidBodies.end(); ++i)
	{
		i->EulerStep(deltaTime);
	}

	last_update_time = sutil::currentTime();
}

void Scene::UpdateCamera()
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

	float3 ray_direction_1 = normalize(-0.5*camera_u + camera_w);
	float3 ray_direction_2 = normalize(0.5*camera_u + camera_w);
	float fov = acos(dot(ray_direction_1, ray_direction_2));

	context["fov"]->setFloat(fov);
}

void Scene::ResolveCollisions()
{
	Buffer responseBuffer = GetResponseBuffer();
	float volume = 0.0f;
	float k = 250.0f;

	IntersectionResponse* responseData = (IntersectionResponse*)responseBuffer->map();
	int physicsPixels = width * height / physicsRayStep / physicsRayStep;
	for(uint i = 0; i < physicsPixels; i++)
	{
		IntersectionResponse response = responseData[i];
		if (responseData[i].volume > 0.00001f)
		{
			float volumeConstraint = response.volume;

			// Apply force at collision entry
			sceneRigidBodies[response.entryId].AddImpulseAtPosition(-response.entryNormal * volumeConstraint * k, response.entryPoint);
			int otherId = response.collisionId == response.entryId ? response.exitId : response.collisionId;
			sceneRigidBodies[otherId].AddImpulseAtPosition(response.entryNormal * volumeConstraint * k, response.entryPoint);

			// Apply force at collision exit
			sceneRigidBodies[response.exitId].AddImpulseAtPosition(-response.exitNormal * volumeConstraint * k, response.exitPoint);
			otherId = response.collisionId;
			sceneRigidBodies[otherId].AddImpulseAtPosition(response.exitNormal * volumeConstraint * k, response.exitPoint);

			volume += response.volume;
		}
	}
	responseBuffer->unmap();

	DisplayGUI(volume);
}

/*
	Glut stuff
*/

void Scene::DisplayGUI(float volume)
{
	// Display intersection volume
	char volumeText[64];

	// Display collision detection
	if (volume > 0.0f)
	{
		char* collisionText = "Rigidbody collision detected!";
		sutil::displayText(collisionText, 25, height-25);
	}

	char* collisionText = "Intersection volume";
	sutil::displayText(collisionText, 25, height-45);
	snprintf(volumeText, sizeof volumeText, "%f", volume);
	sutil::displayText(volumeText, 25, height-65);

	// Display frames per second
	sutil::displayFps(frame_count++);

	if (volume > 0.0f)
	{
		std::cout << volume << std::endl;
	}
}

void Scene::GlutInitialize(int* argc, char** argv)
{
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow(PROJECT_NAME);
	glutHideWindow();
}

void Scene::GlutRun()
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
	glutDisplayFunc(Scene::GlutDisplay);
	glutIdleFunc(Scene::GlutDisplay);
	glutReshapeFunc(Scene::GlutResize);
	glutKeyboardFunc(Scene::GlutKeyboardPress);
	glutMouseFunc(Scene::GlutMousePress);
	glutMotionFunc(Scene::GlutMouseMotion);
	glutCloseFunc(Scene::DestroyContext);

	glutMainLoop();
}

void Scene::GlutDisplay()
{
	Scene instance = Scene::Get();
	instance.UpdateGeometry();
	instance.UpdateCamera();

	instance.context->launch(0, width, height);

	Buffer renderBuffer = instance.GetOutputBuffer();
	sutil::displayBufferGL(renderBuffer);

	instance.ResolveCollisions();

	glutSwapBuffers();
}

void Scene::GlutKeyboardPress(unsigned char k, int x, int y)
{
	Scene instance = Scene::Get();

	switch (k)
	{
		case('q'):
		case(27): // ESC
		{
			Scene::DestroyContext();
			exit(0);
		}
		case('s'):
		{
			const std::string outputImage = std::string(PROJECT_NAME) + ".ppm";
			std::cerr << "Saving current frame to '" << outputImage << "'\n";
			sutil::displayBufferPPM(outputImage.c_str(), instance.GetOutputBuffer());
			break;
		}
	}
}

void Scene::GlutMousePress(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		mouse_button = button;
		mouse_prev_pos = make_int2(x, y);
	}
}

void Scene::GlutMouseMotion(int x, int y)
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

void Scene::GlutResize(int w, int h)
{
	Scene instance = Scene::Get();

	if (w == (int)width && h == (int)height) return;

	width = w;
	height = h;
	sutil::ensureMinimumSize(width, height);

	sutil::resizeBuffer(instance.GetOutputBuffer(), width, height);

	glViewport(0, 0, width, height);

	glutPostRedisplay();
}
