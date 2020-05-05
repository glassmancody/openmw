#version 120

varying vec2 diffuseMapUV;
varying vec3 passViewPos;
varying float depth;

void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

//     gl_Position.z = 1;

    depth = gl_Position.z;
    passViewPos = (gl_ModelViewMatrix * gl_Vertex).xyz;
    diffuseMapUV = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
}
