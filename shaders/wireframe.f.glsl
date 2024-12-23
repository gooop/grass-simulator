// *** Insert appropriate uniforms for the Blinn-Phong model ***
#version 410 core

uniform WorldInfo {
    mat4 mvpMtx;    // precomputed MVP Matrix
    float time;     // current time within our program
    vec3 cameraPos;
};

uniform Lights {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform float Width;
uniform vec4 Color;
uniform bool showWireframe;
uniform mat4 ModelViewMatrix;

layout (location = 0) in vec3 GPosition;
/* layout (location = 1) in vec3 GModelPos; */
layout (location = 2) in vec3 GNormal;
layout (location = 3) noperspective in vec3 GEdgeDistance;
layout( location = 0 ) out vec4 FragColor;

vec3 phongModel( vec3 fpos, vec3 n ) {
  vec3 s = normalize( (ModelViewMatrix * vec4(position, 1)).xyz - fpos );
  float sDotN = max( dot(s,n), 0.0 );
  vec3 diff = diffuse * sDotN;
  vec3 spec = vec3(0.0);
  if( sDotN > 0.0 ) {
    vec3 v = normalize(-fpos.xyz);
    vec3 r = reflect( -s, n );
    spec = specular *
            pow( max( dot(r,v), 0.0 ), 32);
  }
  return ambient * vec3(1, 0, 0) + 1 * (diff + spec);
}

void main() {
    // The shaded surface color.
    vec4 color=vec4(phongModel(GPosition, GNormal), 1.0);
    // Find the smallest distance
    float d = min( GEdgeDistance.x, GEdgeDistance.y );
    d = min( d, GEdgeDistance.z );
    // Determine the mix factor with the line color
    float mixVal = smoothstep( Width - 1,
                               Width + 1, d );
    if (!showWireframe) {
        mixVal = 1;
    }
    // Mix the surface color with the line color
    FragColor = mix( Color, color, mixVal );
}
