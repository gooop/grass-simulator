/*
 *  CSCI 444, Advanced Computer Graphics, Spring 2021
 *
 *  Project: lab3
 *  File: bezierPatch.tc.glsl
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
#version 410 core

// TODO A - specify the number of output vertices
layout(vertices=3) out;


// ***** TESSELLATION CONTROL SHADER UNIFORMS *****


// ***** TESSELLATION CONTROL SHADER INPUT *****

layout (location = 0 ) in vec3 ControlPoints[];

// ***** TESSELLATION CONTROL SHADER OUTPUT *****

out vec4 ControlPointsOut[];

// ***** TESSELLATION CONTROL SHADER SUBROUTINES *****


// ***** TESSELLATION CONTROL SHADER HELPER FUNCTIONS *****
/* in gl_PerVertex */
/* { */
/*   vec4 gl_Position; */
/* } gl_in[gl_MaxPatchVertices]; */

/* out gl_PerVertex */
/* { */
/*   vec4 gl_Position; */
/* } gl_out[]; */


// ***** TESSELLATION CONTROL SHADER MAIN FUNCTION *****
void main() {
    // TODO B - pass the control point through
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    ControlPointsOut[gl_InvocationID] = vec4(ControlPoints[gl_InvocationID], 1);


    // TODO C - set the outer and inner tessellation levels
    if (gl_InvocationID == 0) {
        gl_TessLevelOuter[0] = 15;
        gl_TessLevelOuter[1] = 15;
        gl_TessLevelOuter[2] = 15;
        gl_TessLevelOuter[3] = 15;
        gl_TessLevelInner[0] = 15;
        gl_TessLevelInner[1] = 15;
    }


}

