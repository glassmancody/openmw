#version 120

uniform sampler2D diffuseMap;
varying vec3 passViewPos;
uniform mat4 osg_ViewMatrixInverse;

varying vec2 diffuseMapUV;
varying vec4 passColor;

#include "world.glsl"
#include "sunlight.glsl"

void applyFog(vec3 cameraPos, vec3 worldPos);

void main(void)
{
    const float scale = 1.0;

    gl_FragData[0] = texture2D(diffuseMap, diffuseMapUV * scale);

    vec3 worldPos = (osg_ViewMatrixInverse * vec4(passViewPos.xyz, 1)).xyz;
    vec3 objPos = worldPos - world.cameraPos;
    vec3 dir = normalize(objPos);
    objPos = dir * 500 * 1000 * 100;
    worldPos = objPos + world.cameraPos;

    applyFog(world.cameraPos, worldPos);
}
