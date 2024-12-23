#version 430 core

layout(local_size_x = 1, local_size_y = 1) in;

uniform float dt;
uniform float t;

layout(std430, binding=10) buffer v0Ssbo{
    vec4 v0s[];
};
layout(std430, binding=11) buffer v1Ssbo{
    vec4 v1s[];
};
layout(std430, binding=12) buffer v2Ssbo{
    vec4 v2s[];
};
layout(std430, binding=13) buffer iv2Ssbo{
    vec4 iv2s[];
};
layout(std430, binding=14) buffer frontSsbo{
    vec4 fronts[];
};
layout(std430, binding=15) buffer nSsbo{
    vec4 ns[];
};
layout(std430, binding=16) buffer lengthSsbo{
    vec4 lengths[];
};

float newN(float oldN) {
    float a = 1;
    return max(oldN - a * dt, 0);
}

vec3 recovery(vec3 iv2, vec3 v2, float n) {
    float s = .3;
    return (iv2 - v2) * s * max(1 - n, 0.1);
}

vec3 gravity(vec3 f) {
    float m = 0.1;
    float dmag = .05;
    vec3 ddir = vec3(0, -1, 0);
    // Only accounting for direction
    vec3 ge = ddir * dmag;
    vec3 gf = 0.25 * dmag * f;
    return ge + gf;
}

vec3 wind(vec3 v0, vec3 v2, float h) {
    vec3 wi = vec3(sin((t + v0.x) * 0.6), 0, cos((t + v0.z) * 0.6));
    float fd = 1 - abs(dot(normalize(wi), (v2 - v0) / length(v2 - v0)));
    float fr = dot(v2 - v0, vec3(0, 1, 0)) / h;
    float theta = fd * fr;
    return wi * theta;
}

vec3 aboveGround(vec3 v0, vec3 v2) {
    return v2 - vec3(0, 1, 0) * min(vec3(0, 1, 0) * (v2 - v0), 0);
}

vec3 v1Loc(vec3 v0, vec3 v2, float h) {
    float lproj = length(v2 - v0 - vec3(0, 1, 0) * dot(v2 - v0, vec3(0, 1, 0)));
    return v0 + h * vec3(0, 1, 0) * max(1 - lproj/h, 0.05 * max(lproj/h, 1));
}

float corrective(vec3 v0, vec3 v1, vec3 v2, float h) {
    uint n = 2; // I think this is the degree?
    float l0 = length(v2 - v0);
    float l1 = length(v1 - v0) + length(v2 - v1);
    float l = (2 * l0 + (n - 1) * l1) / (n + 1);
    return h / l;
}

vec3 collision(vec3 v2) {
    vec3 c = vec3(3 * cos(t), 3, -7);
    vec3 d = min(length(c - v2) - 3, 0) * normalize(c - v2);
    return d;
}

void main() {
    uint i = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * 400;
    vec3 v0 = v0s[i].xyz;
    vec3 v2 = v2s[i].xyz;
    vec3 iv2 = iv2s[i].xyz;
    vec3 front = fronts[i].xyz;
    float n = ns[i].x;
    float h = lengths[i].x;
    vec3 delta = (recovery(iv2, v2, n) + gravity(front) + wind(v0, v2, h)) * dt;

    // To test the individual components on their own, change this line.
    /* vec3 delta = (recovery(iv2, v2, n) + gravity(front) + wind(v0, v2, h)) * dt + vec3(0, 0, 0); //No collisions for now; */
    v2 = v2 + delta;
    vec3 d = collision(v2);
    v2 = v2 + d;
    v2 = aboveGround(v0, v2);
    vec3 v1 = v1Loc(v0, v2, h);
    float r = corrective(v0, v2, v1, h);
    v1 = v0 + r * (v1 - v0);
    v2 = v1 + r * (v2 - v1);
    v2s[i] = vec4(v2, 0);
    v1s[i] = vec4(v1, 0);
    ns[i] = vec4(newN(n) + pow(length(d), 2), 0, 0, 20);
}
