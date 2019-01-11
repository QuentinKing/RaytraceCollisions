This directory contains binaries for Chris Wyman's shader tutorials from the SIGGRAPH 2018 Course 
"Introduction to DirectX Raytracing."  Please visit the course webpage (http://intro-to-dxr.cwyman.org) 
or Chris' webpage (http://cwyman.org) to get more details, source code, more descriptive 
tutorial walkthroughs, course presentations, and other information about the DXR course.

Please read this document carefully before running our tutorials.  Please contact Chris if you 
run into unexpected troubles not covered below.

Also note:  None of these tutorials are intended to demonstrate best practices for highly
optimized ray tracing performance.  These samples are optimized to provide an easy starting point, 
clarity, and general code readability (rather than performance).

----------------------------------------------------------------------------------------------
 Requirements:
----------------------------------------------------------------------------------------------

1) Windows 10 RS5 or later.
     * If you run "winver.exe" you should have Version 1809 (OS Build 17763)
     * The tutorials do *not* run on Win 10 RS4 or earlier
     * If you have Windows 10 RS4, please see my webpage for binaries that run on that OS.

2) A graphics card supporting DirectX Raytracing
     * I have tested on GeForce RTX cards, and a Titan V should work.
     * Due to time constraints, I have not had a chance to test use of the fallback layer

3) A driver that natively supports DirectX Raytracing
     * For NVIDIA, use 416.xx or later drivers (in theory any NVIDIA driver for RS5 should work)

----------------------------------------------------------------------------------------------
Loading scenes:
----------------------------------------------------------------------------------------------

This .zip file includes one scene (the "modern living room" from Benedikt Bitterli's page), 
which loads automatically for all tutorials that require a scene.  If you would like to test
these samples on more complex geoemtry, you can download one of the other scenes from the 
Open Research Content Archive:
     * https://developer.nvidia.com/orca 
     * https://developer.nvidia.com/orca/amazon-lumberyard-bistro
     * https://developer.nvidia.com/orca/nvidia-emerald-square
     * https://developer.nvidia.com/ue4-sun-temple


----------------------------------------------------------------------------------------------
Acknowledgements:
----------------------------------------------------------------------------------------------

The desert HDR environment map (MonValley Dirtroad) is provided from the sIBL Archive under a 
Creative Commons license (CC BY-NC-SA 3.0 US). See
    http://www.hdrlabs.com/sibl/archive.html 

The included "pink_room" scene is named 'The Modern Living Room' by Wig42 on Benedikt Bitterli's
webpage (https://benedikt-bitterli.me/resources/).  It has been modified to match the Falcor 
material system.  This scene was released under a CC-BY license.  It may be copied, modified and 
used commercially without permission, as long as: Appropriate credit is given to the original author
The original scene file may be obtained here: 
    http://www.blendswap.com/blends/view/75692