#version 430 compatibility

varying vec2 diffuseMapUV;

varying float alphaPassthrough;

uniform bool useTreeAnim;
uniform bool useDiffuseMapForShadowAlpha = true;
uniform bool alphaTestShadows = true;

#include "lib/core/vertex.h.glsl"
#include "lib/material/colormodes.glsl"

void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    vec4 viewPos = (gl_ModelViewMatrix * gl_Vertex);
    gl_ClipVertex = viewPos;

    if (useDiffuseMapForShadowAlpha)
        diffuseMapUV = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
    else
        diffuseMapUV = vec2(0.0); // Avoid undefined behaviour if running on hardware predating the concept of dynamically uniform expressions

    assignMaterials();
    Material material = getMaterial();

    if (material.colorMode == ColorMode_AmbientAndDiffuse)
        alphaPassthrough = useTreeAnim ? 1.0 : gl_Color.a;
    else
        // This is uniform, so if it's too low, we might be able to put the position/clip vertex outside the view frustum and skip the fragment shader and rasteriser
        alphaPassthrough = gl_FrontMaterial.diffuse.a;
}
