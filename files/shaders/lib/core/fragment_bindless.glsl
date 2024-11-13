#version 430 core

#extension GL_NV_bindless_texture : require
#extension GL_NV_gpu_shader5 : require

#include "lib/material/bindings.glsl"
#include "lib/material/material.glsl"
#include "lib/texture/bindings.glsl"

flat in int materialIndex;
flat in int diffuseMapIndex;

Material getMaterial()
{
    return materials[materialIndex];
}

#if @diffuseMap
uniform sampler2D diffuseMap;
vec4 sample_diffuse(in vec2 uv)
{
    // return texture(diffuseMap, uv);
    return texture(textures[diffuseMapIndex], uv);
}
#endif
