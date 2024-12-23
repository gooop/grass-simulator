#version 430 core

layout(local_size_x = 1) in;

layout(std430, binding=0) buffer SSBO{
    vec4 locations[100];
};

void main() {
    float x = gl_GlobalInvocationID.x;
    float old = locations[gl_GlobalInvocationID.x].y;
    float dx = locations[gl_GlobalInvocationID.x].x;
    if (old > 1) {
        locations[gl_GlobalInvocationID.x] = vec4(-0.001 * x, 0.999, 0, 0);
    } else if (old <= 0) {
        locations[gl_GlobalInvocationID.x] = vec4(0.001 * x, 0.0001, 0, 0);
    } else {
        locations[gl_GlobalInvocationID.x] = locations[gl_GlobalInvocationID.x] + vec4(0, dx, 0, 0);
    }
}
