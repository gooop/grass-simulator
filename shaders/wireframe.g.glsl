#version 410 core

layout( triangles ) in;
layout( triangle_strip, max_vertices = 3 ) out;
layout (location=2 ) out vec3 GNormal;
layout ( location=0) out vec3 GPosition;
layout (location =1) out vec3 GModelPos;
layout ( location=3) noperspective out vec3 GEdgeDistance;
layout (location = 0 ) in vec3 VNormal[];
layout (location = 1 ) in vec3 VPosition[];
layout ( location = 2) in vec3 ModelPos[];


in gl_PerVertex
{
  vec4 gl_Position;
} gl_in[];

out gl_PerVertex
{
    vec4 gl_Position;
};

uniform mat4 ViewportMatrix;  // Viewport matrix

void main() {
    // Transform each vertex into viewport space
    vec3 p0 = vec3(ViewportMatrix * (gl_in[0].gl_Position /
                            gl_in[0].gl_Position.w));
    vec3 p1 = vec3(ViewportMatrix * (gl_in[1].gl_Position /
                             gl_in[1].gl_Position.w));
    vec3 p2 = vec3(ViewportMatrix * (gl_in[2].gl_Position /
                            gl_in[2].gl_Position.w));
        // Find the altitudes (ha, hb and hc)
    float a = length(p1 - p2);
    float b = length(p2 - p0);
    float c = length(p1 - p0);
    float alpha = acos( (b*b + c*c - a*a) / (2.0*b*c) );
    float beta = acos( (a*a + c*c - b*b) / (2.0*a*c) );
    float ha = abs( c * sin( beta ) );
    float hb = abs( c * sin( alpha ) );
    float hc = abs( b * sin( alpha ) );
    // Send the triangle along with the edge distances
    GEdgeDistance = vec3( ha, 0, 0 );
    GNormal = VNormal[0];
    GPosition = VPosition[0];
    GModelPos = ModelPos[0];
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    GEdgeDistance = vec3( 0, hb, 0 );
    GNormal = VNormal[1];
    GPosition = VPosition[1];
    GModelPos = ModelPos[1];
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();
    GEdgeDistance = vec3( 0, 0, hc );
    GNormal = VNormal[2];
    GPosition = VPosition[2];
    GModelPos = ModelPos[2];
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    EndPrimitive();
}
