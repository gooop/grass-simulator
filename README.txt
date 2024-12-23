# Grass Simulating Shaders

Hello! It has been a long time since I have compiled this, and I compiled it with CLion, which I have no current license for. So, I'm going to give a picture and directions to the interesting code.

The C++ side was mostly written by my professor at the time, however, the function setupBuffers() was written by me, in order to fill the vertex, frame, and texture buffers with useful information for the shader. The shader to be interested in is the "physicalGrass.glsl" shader, written by my partner and I. There are several functions within this shader to simulate wind, gravity, deformation, etc. There is a sphere that is not being rendered (for grass visibility) that deforms the grass as well.

Here's a picture of the grass being deformed by wind, gravity, and the sphere:
![Square field of grass being deformed](/assets/pics/final.png)
