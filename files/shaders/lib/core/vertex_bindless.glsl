#version 430 core

#extension GL_NV_bindless_texture : require
#extension GL_NV_gpu_shader5 : require

#include "lib/material/bindings.glsl"
#include "lib/material/material.glsl"

layout(location = 7) in vec4 vMaterialIndex;
flat out int materialIndex;
flat out int diffuseMapIndex;

void assignMaterials()
{
    materialIndex = int(vMaterialIndex.x);
    diffuseMapIndex = int(vMaterialIndex.y);
}

Material getMaterial()
{
    return materials[materialIndex];
}
