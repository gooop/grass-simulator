/*
 *  CSCI 444, Advanced Computer Graphics, Spring 2019
 *
 *  Project: lab2
 *  File: colorPassThrough.f.glsl
 *
 *  Description:
 *      Lab2
 *
 *      Pass Through Shader to apply color
 *
 *  Author:
 *      Dr. Jeffrey Paone, Colorado School of Mines
 *
 *  Notes:
 *
 */

// we are using OpenGL 4.1 Core profile
#version 420 core

// ***** UNIFORMS *****
uniform WorldInfo {
    mat4 mvpMtx;    // precomputed MVP Matrix
    float time;     // current time within our program
    vec3 cameraPos;
    mat4 vpMtx;    // precomputed VP Matrix
};

uniform Lights {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// ***** FRAGMENT SHADER INPUT *****
in vec3 normal;
in vec3 fPos;

// ***** FRAGMENT SHADER OUTPUT *****
layout (location = 0) out vec4 gPositionOut;
layout (location = 1) out vec4 gNormalOut;
layout (location = 2) out vec4 gColorSpecOut;

// Credit to https://learnopengl.com/Lighting/Basic-Lighting for a lot of help
// with the Phong formulas.
void main() {
    gPositionOut = vec4(fPos, 1.0);
    /* gPositionOut = vec3(1.f, 1.f, 1.f); */
    gNormalOut = vec4(normalize(normal), 1.0);
    gColorSpecOut = vec4(1.0, 0, 0, 1.0);

    if( !gl_FrontFacing ) {
        gColorSpecOut.rgb = gColorSpecOut.bgr;
    }
}
