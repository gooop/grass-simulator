/*
 *  CSCI 444, Advanced Computer Graphics, Spring 2021
 *
 *  Project: lab3
 *  File: bezierPatch.te.glsl
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

// TODO D - specify the primitive type, point spacing, and winding order
layout(quads, equal_spacing, cw) in;


// ***** TESSELLATION EVALUATION SHADER UNIFORMS *****
/* uniform WorldInfo { */
/*     mat4 mvpMtx;    // precomputed MVP Matrix */
/*     float time;     // current time within our program */
/*     vec3 cameraPos; */
/* }; */
uniform mat4 mvpMtx;    // precomputed MVP Matrix
/* uniform mat4 ModelViewMatrix; */
/* uniform mat3 NormalMatrix; */


// ***** TESSELLATION EVALUATION SHADER INPUT *****

in vec4 ControlPointsOut[];
in gl_PerVertex
{
  vec4 gl_Position;
} gl_in[gl_MaxPatchVertices];

// ***** TESSELLATION EVALUATION SHADER OUTPUT *****
// TODO J - send patch point to the fragment shader

layout (location = 0 ) out vec3 VNormal;
layout (location = 1 ) out vec3 VPosition;
/* layout (location = 2 ) out vec3 ModelPos; */

out gl_PerVertex {
  vec4 gl_Position;
};

// ***** TESSELLATION EVALUATION SHADER SUBROUTINES *****


// ***** TESSELLATION EVALUATION SHADER HELPER FUNCTIONS *****
// TODO G - create a function to perform the bezier curve calculation
vec4 c(float v, vec4 v0, vec4 v1, vec4 v2) {
    vec4 a = v0 + v * (v1 - v0);
    vec4 b = v1 + v * (v2 - v1);
    return a + v * (b - a);
}

// ***** TESSELLATION EVALUATION SHADER MAIN FUNCTION *****
void main() {
    // TODO E - get our tessellation coordinate as u & v
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    // TODO F - get the control points
    vec4 v0 = ControlPointsOut[0];
    vec4 v1 = ControlPointsOut[1];
    vec4 v2 = ControlPointsOut[2];

    vec4 c0 = c(v, v0, v1, v2) + 0.03 * vec4(1.0, 0, 0, 0);
    vec4 c1 = c(v, v0, v1, v2) - 0.03 * vec4(1.0, 0, 0, 0);
    vec4 position = c0 * u + (1 - u) * c1;

    gl_Position = mvpMtx * position;
    VPosition = position.xyz;
    /* ModelPos = f.xyz; */
    VNormal = normalize(vec3(0, 0, 1));

    // TODO K - set our patch point
}

