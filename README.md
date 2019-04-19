# Raytrace Collisions
This repo is the source code I wrote for my CSC494 project course at the University of Toronto. This project is a novel approach to computing collision detection and response between rigidbodies by doing image based collision detection at the same time as a scene is rendered through raytracing. Essentially, we perform rendering and collision detection at the same time, by calculating the volume between objects in the scene using the same rays we are already casting for rendering. By specifying a soft volume contrainst, we get an approximated collision response with little overhead. Written using NVIDIA's raytracing engine OptiX.

### Reference Video
Check out some of the collision responses in action! https://www.youtube.com/watch?v=DVNYyMbgs4A&feature=youtu.be

### Dependencies 
To run this project you'll still you download / install a few dependencies:
  
NVIDIA Optix SDK: https://developer.nvidia.com/optix  
NVIDIA CUDA Toolkit: https://developer.nvidia.com/cuda-downloads  
CMake: https://cmake.org/download/  
  
### Setup
- Copy over the 'bin64', 'include', and 'lib64' folder (ie Optix SDK source files) from Optix's install directory over into this project's OptiX folder.
- In CMake, specify source code as the OptiX/src folder
- In CMake, create some folder to store the generated solution (I usually just make it OptiX/src/build)
- Configure and generate the project for Visual Studio, open the generated solution file and run the 'CSC494' project
  
### Special Thanks
My supervising professor, David Levin (http://diwlevin.webfactional.com/researchdb/)  
HDR Skybox courtsey of Greg Zaal and https://hdrihaven.com  
