/*
 *  CSCI 444, Advanced Computer Graphics, Spring 2021
 *
 *  Project: lab3
 *  File: bezierPatch.f.glsl
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

// ***** FRAGMENT SHADER UNIFORMS *****


// ***** FRAGMENT SHADER INPUT *****
// TODO L - receive the patch point

layout (location = 0 ) in vec3 VNormal;
layout (location = 1 ) in vec3 VPosition;

// ***** FRAGMENT SHADER OUTPUT *****
layout (location = 0) out vec4 gPositionOut;
layout (location = 1) out vec4 gNormalOut;
layout (location = 2) out vec4 gColorSpecOut;

// ***** FRAGMENT SHADER SUBROUTINES *****


// ***** FRAGMENT SHADER HELPER FUNCTIONS *****


// ***** FRAGMENT SHADER MAIN FUNCTION *****
void main() {
	// TODO M - set the color to the patch point
	gColorSpecOut = vec4( 0.3 * VPosition.y, 0.5 * VPosition.y + 0.2, 0, 1 );

	// These positions and normals are only used for lighting and shouldn't
	// affect where the object is located.
	gPositionOut = vec4(VPosition, 1);
	gNormalOut = vec4(VNormal, 1);

	// if viewing the backside of the fragment, 
	// reverse the colors as a visual cue
	/* if( !gl_FrontFacing ) { */
	/* 	fragColorOut.rgb = fragColorOut.bgr; */
	/* } */
}
