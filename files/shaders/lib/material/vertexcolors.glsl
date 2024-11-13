#ifndef LIB_MATERIAL_PROPERTIES_H
#define LIB_MATERIAL_PROPERTIES_H

#include "lib/material/material.glsl"

centroid varying vec4 passColor;

vec4 getEmissionColor(Material material)
{
    if (material.colorMode == ColorMode_Emission)
        return passColor;
    return material.emission;
}

vec4 getAmbientColor(Material material)
{
    if (material.colorMode == ColorMode_AmbientAndDiffuse || material.colorMode == ColorMode_Ambient)
        return passColor;
    return material.ambient;
}

vec4 getDiffuseColor(Material material)
{
    if (material.colorMode == ColorMode_AmbientAndDiffuse || material.colorMode == ColorMode_Diffuse)
        return passColor;
    return material.diffuse;
}

vec4 getSpecularColor(Material material)
{
    if (material.colorMode == ColorMode_Specular)
        return passColor;
    return material.specular;
}

#endif