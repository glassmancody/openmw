#ifndef LIB_MATERIAL_BINDINGS
#define LIB_MATERIAL_BINDINGS

#include "lib/material/material.glsl"

layout(std430, binding = 1) buffer materialsSSBO
{
    Material materials[];
};

#endif