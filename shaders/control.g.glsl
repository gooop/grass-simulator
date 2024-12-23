#version 410 core

layout( points ) in;
layout( triangle_strip, max_vertices = 3 ) out;

in gl_PerVertex
{
  vec4 gl_Position;
} gl_in[];

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() {
    gl_Position = gl_in[0].gl_Position + vec4(0, 0.05, 0, 0);
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(0.05, -0.05, 0, 0);
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(-0.05, -0.05, 0, 0);
    EmitVertex();
    EndPrimitive();
}

