#ifndef LIB_TEXTURE_BINDINGS
#define LIB_TEXTURE_BINDINGS

layout(std430, binding = 2) buffer texturesSSBO
{
    sampler2D textures[];
};

#endif