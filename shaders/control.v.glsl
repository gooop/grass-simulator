#version 410 core

layout (location = 0 ) in vec3 VertexPosition;

uniform WorldInfo {
    mat4 mvpMtx;    // precomputed MVP Matrix
    float time;     // current time within our program
    vec3 cameraPos;
};

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() {
    gl_Position = mvpMtx * vec4(VertexPosition,1.0);
}

