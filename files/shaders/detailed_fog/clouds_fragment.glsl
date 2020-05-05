#version 120

#include "world.glsl"
#include "sunlight.glsl"

uniform sampler2D diffuseMap;

varying vec2 diffuseMapUV;

void applyFog();

//TODO: direct sunlight

void main(void)
{
    const float scale = 4.0;

    gl_FragData[0] = texture2D(diffuseMap, diffuseMapUV * scale);

    vec3 opaqueColor = gl_FragData[0].xyz * ambientLight;
    clamp(opaqueColor, 0, 1);
    float opacity = pow(gl_FragData[0].a, 2);
    opacity = 1;
    gl_FragData[0].xyz = mix(gl_FragData[0].xyz, opaqueColor, opacity);

    applyFog();

}
