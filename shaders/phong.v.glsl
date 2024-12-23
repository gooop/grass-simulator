/*
 *  CSCI 444, Advanced Computer Graphics, Spring 2021
 *
 *  Project: lab2
 *  File: colorPassThrough.v.glsl
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
#version 430 core

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

// ***** ATTRIBUTES *****
layout(location=0) in vec3 vPos;           // vertex position
layout(location=1) in vec3 vNormal;         // vertex color
layout(location=2) in vec3 loc;

// ***** VERTEX SHADER OUTPUT *****
out vec3 normal;         // color to apply to this vertex
out vec3 fPos;

// ***** VERTEX SHADER SUBROUTINES *****

layout(std430, binding=0) readonly buffer SSBO{
    vec4 locations[100];
};

void main() {
    // transform our vertex into clip space
    gl_Position = mvpMtx * vec4(vPos + loc + locations[gl_InstanceID].xyz, 1);

    
    // pass the color through
    normal = vNormal;
    fPos = vPos;
}
