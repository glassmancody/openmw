#version 120

#include "world.glsl"
#include "sunlight.glsl"
#include "sunlight_calculation.glsl"

varying vec2 diffuseMapUV;
varying vec3 passViewPos;
varying float depth;

void main(void)
{
  calcSunLightParams();

    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    depth = gl_Position.z;
    passViewPos = (gl_ModelViewMatrix * gl_Vertex).xyz;
    diffuseMapUV = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
}
