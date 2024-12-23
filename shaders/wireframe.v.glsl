#version 410 core

layout (location = 0 ) in vec3 VertexPosition;
layout (location = 1 ) in vec3 VertexNormal;
layout (location = 0 ) out vec3 VNormal;
layout (location = 1 ) out vec3 VPosition;
layout (location = 2 ) out vec3 VPos;

out gl_PerVertex
{
    vec4 gl_Position;
};

uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;
uniform WorldInfo {
    mat4 mvpMtx;    // precomputed MVP Matrix
    float time;     // current time within our program
    vec3 cameraPos;
};

void main() {
    VNormal = normalize( NormalMatrix * VertexNormal);
    VPosition = vec3(ModelViewMatrix * vec4(VertexPosition,1.0));
    VPos = vec3(VertexPosition);
    gl_Position = mvpMtx * vec4(VertexPosition,1.0);
}
