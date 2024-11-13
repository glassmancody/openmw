@link "lib/core/vertex.glsl" if !@useOVR_multiview
@link "lib/core/vertex_multiview.glsl" if @useOVR_multiview
@link "lib/core/vertex_bindless.glsl" if !@legacyBindings

vec4 modelToClip(vec4 pos);
vec4 modelToView(vec4 pos);
vec4 viewToClip(vec4 pos);

#include "lib/material/material.glsl"

void assignMaterials();

Material getMaterial();
