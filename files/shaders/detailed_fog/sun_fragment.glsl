#version 120

uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;

varying vec3 passViewPos;
uniform mat4 osg_ViewMatrixInverse;

#include "world.glsl"

void applyFog(vec3 cameraPos, vec3 worldPos);
float calcSunVisibility(vec3 cameraPos, vec3 worldPos);

void main(void)
{
    gl_FragData[0] = texture2D(diffuseMap, diffuseMapUV) * gl_LightSource[0].diffuse * 1.3;

    vec3 worldPos = (osg_ViewMatrixInverse * vec4(passViewPos.xyz, 1)).xyz;
    vec3 objPos = worldPos - world.cameraPos;
    objPos *= 15000;
    worldPos = objPos + world.cameraPos;

    gl_FragData[0].a *= calcSunVisibility(world.cameraPos, worldPos);
}
