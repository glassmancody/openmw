#ifndef OPENMW_FRAGMENT_H_GLSL
#define OPENMW_FRAGMENT_H_GLSL

@link "lib/core/fragment.glsl" if !@useOVR_multiview
@link "lib/core/fragment_multiview.glsl" if @useOVR_multiview
@link "lib/core/fragment_bindless.glsl" if !@legacyBindings

#include "lib/material/material.glsl"

vec4 sampleReflectionMap(vec2 uv);

#if @waterRefraction
vec4 sampleRefractionMap(vec2 uv);
float sampleRefractionDepthMap(vec2 uv);
#endif

vec4 samplerLastShader(vec2 uv);

#if @skyBlending
vec3 sampleSkyColor(vec2 uv);
#endif

Material getMaterial();

#if @diffuseMap
    vec4 sample_diffuse(in vec2 uv);
#endif

#endif  // OPENMW_FRAGMENT_H_GLSL
