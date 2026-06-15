#version 120

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

uniform sampler2D diffuseMap;

varying vec2 diffuseMapUV;
varying vec4 passColor;

vec2 sqr(vec2 x)
{
    return x * x;
}

float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float screenPxRange()
{
#if @useGPUShader4
    vec2 size = textureSize2D(diffuseMap, 0).xy;
#else
    vec2 size = vec2(256.0);
#endif

    const float pxRange = 4.0;

    vec2 unitRange = vec2(pxRange) / size;
    vec2 screenTexSize = vec2(1.0) / sqrt(sqr(dFdx(diffuseMapUV)) + sqr(dFdy(diffuseMapUV)));
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

void main()
{
    vec4 msdf = texture2D(diffuseMap, diffuseMapUV);

    float sd = median(msdf.r, msdf.g, msdf.b);
    float screenPxDistance = screenPxRange() * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    gl_FragData[0] = vec4(passColor.rgb, passColor.a * opacity * msdf.a);
}
