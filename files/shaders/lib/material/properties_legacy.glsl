#version 120

#include "lib/material/colormodes.glsl"
#include "lib/material/vertexcolors.glsl"

uniform float specStrength;
uniform float emissiveMult;
uniform int colorMode;

Material getMaterial(int index)
{
    Material material;

    material.diffuse = gl_FrontMaterial.diffuse;
    material.ambient = gl_FrontMaterial.emission;
    material.specular = gl_FrontMaterial.emission;
    material.emission = gl_FrontMaterial.emission;
    material.shininess = gl_FrontMaterial.shininess;
    material.emissiveMult = emissiveMult;
    material.specStrength = specStrength;
    material.colorMode = colorMode;

    return material;
}
