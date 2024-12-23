/*
 *  CSCI 444, Advanced Computer Graphics, Spring 2021
 *
 *  Project: lab3
 *  File: bezierPatch.v.glsl
 *
 *  Description:
 *      Renders a patch based off of control points
 *
 *  Author:
 *      Dr. Jeffrey Paone, Colorado School of Mines
 *
 *  Notes:
 *
 */

// we are using OpenGL 4.1 Core profile
#version 430 core

// ***** VERTEX SHADER UNIFORMS *****

// ***** VERTEX SHADER INPUT *****
/* layout (location = 0 ) in uint i; */
/* layout (location = 1 ) in uint j; */
layout (location = 0) out vec3 ControlPoints;

// ***** VERTEX SHADER OUTPUT *****


// ***** VERTEX SHADER SUBROUTINES *****

layout(std430, binding=10) buffer v0Ssbo{
    vec4 v0s[];
};
layout(std430, binding=11) buffer v1Ssbo{
    vec4 v1s[];
};
layout(std430, binding=12) buffer v2Ssbo{
    vec4 v2s[];
};

// ***** VERTEX SHADER HELPER FUNCTIONS *****


// ***** VERTEX SHADER MAIN FUNCTION *****
void main() {
    uint i = gl_VertexID / 3;
    uint j = gl_VertexID % 3;
    // transform our vertex into clip space
    if (j == 0) {
        ControlPoints = v0s[i].xyz;
    } else if (j == 1) {
        ControlPoints = v1s[i].xyz;
    } else {
        ControlPoints = v2s[i].xyz;
    }

    // I'm pretty sure gl_position gets overwritten by the tessellation stuff.
    /* gl_Position = mvpMtx * vec4( vPos, 1); */
}
