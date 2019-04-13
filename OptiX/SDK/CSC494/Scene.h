#pragma once

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

using namespace optix;

class Scene
{
public:
	static Scene& Get()
    {
        static Scene instance;
        return instance;
    }

	void Setup(int argc, char** argv, std::string out_file, bool use_pbo);

	Buffer GetOutputBuffer();
	Buffer GetRigidbodyMotionBuffer();
	Buffer GetResponseBuffer();

private:
	Scene() {}

	void CreateContext();
	void CreateScene();
	void CreateLights();
	void SetupCamera();
	void UpdateGeometry();
	void UpdateRigidbodyState();
	void UpdateCamera();
	void ResolveCollisions();
	void DisplayGUI(float volume);

	// Static callbacks for GLUT
	static void DestroyContext();
	static void GlutInitialize(int* argc, char** argv);
	static void GlutRun();
	static void GlutDisplay();
	static void GlutKeyboardPress(unsigned char k, int x, int y);
	static void GlutMousePress(int button, int state, int x, int y);
	static void GlutMouseMotion(int x, int y);
	static void GlutResize(int w, int h);

	Context context;
};
