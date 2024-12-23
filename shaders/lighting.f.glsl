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

layout(binding=1) uniform sampler2D gPosition;
layout(binding=2) uniform sampler2D gNormal;
layout(binding=3) uniform sampler2D gColorSpec;

// ***** FRAGMENT SHADER INPUT *****
in vec3 normal;
in vec3 fPos;

// ***** FRAGMENT SHADER OUTPUT *****
out vec4 gColorSpecOut;

// ***** FRAGMENT SHADER SUBROUTINES *****
vec3 phong(vec3 norm) {
    vec3 reflection = normalize(-position + 2*(dot(norm,position))*norm);
    float spec = pow(max(dot(normalize(cameraPos), reflection), 0.0), 4);
    vec3 speced = spec * specular * vec3(0, 1, 0);
    return speced;
}

vec3 blinnphong(vec3 norm) {
    return vec3(0, 0, 0);
    vec3 halfv = normalize(normalize(cameraPos) + normalize(position-fPos));
    float spec = pow(max(dot(norm, halfv), 0.0), 128);
    vec3 speced = spec * specular * vec3(0, 1, 0);
    return speced;
}


// Credit to https://learnopengl.com/Lighting/Basic-Lighting for a lot of help
// with the Phong formulas.
void main() {
    vec3 pos = texture(gPosition, (fPos.xy + 1) / 2).rgb;
    vec3 norm = texture(gNormal, (fPos.xy + 1) / 2).rgb;
    vec4 tex = texture(gColorSpec, (fPos.xy + 1) / 2);

    // set the output color to the interpolated color
    /* vec3 lightDir = normalize(position - pos); */
    /* float diff = max(dot(norm, lightDir), 0.0); */
    /* vec3 diffused = diff * diffuse * vec3(0, 0.5, 0); */

    /* vec3 speced = blinnphong(norm); */
    vec3 color = ambient * tex.rgb; //+ diffused + speced;
    gColorSpecOut = vec4(color, 1);
    /* gColorSpecOut = vec4(1, 0, 0, 1); */

    // if viewing the backside of the fragment, 
    // reverse the colors as a visual cue
}

