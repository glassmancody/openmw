#version 120

varying vec2 diffuseMapUV;
varying vec3 passViewPos;
varying float depth;




// any RGB24 color value multiplied with this factor (or below) will be 0,0,0
#define FOG_ZERO_VISIBILITY_VALUE \
    (1.f / 256.f)

#define FOG_LAYER_DENSITY(viewingDistance) \
    (log(FOG_ZERO_VISIBILITY_VALUE) / -(viewingDistance))

#define FOG_DENSITY_SCALE(height, falloff) \
    exp(-(falloff) * height)
#define FOG_DENSITY_SCALE_ANTIDERIVATIVE(height, falloff) \
    (-(1/falloff) * exp(-falloff * height))

#define FOG_LAYER_FALLOFF(height1) \
    (log(0.5) / -(height1))
#define FOG_FALLOFF_LINEAR(layerTop) \
    (1.f / (layerTop))


vec3 intersect(vec3 lineP,
               vec3 lineN,
               vec3 planeN,
               float  planeD)
{
    float distance = (planeD - dot(planeN, lineP)) / dot(lineN, planeN);
    return lineP + lineN * distance;
}


float calcFogLayerValue(
    float baseHeight,
    float falloff,
    float density,
    vec3 cameraPos,
    vec3 worldPos)
{
    baseHeight /= 100.0;
    cameraPos /= 10000.0;
    worldPos /= 10000.0;

    // FIXME: this function sometimes calculates erroneous values for avgDensityScale -
    // (verified by setting avgDensityScale to a constant value)
    // this manifests itself as one horizontal line of ramdomly colored pixels on screen
    // occurs seldomly and most likely when a few meters above the water line (altitude 0)

    vec3 minPos = cameraPos.z < worldPos.z ? cameraPos : worldPos;
    vec3 maxPos = cameraPos.z < worldPos.z ? worldPos : cameraPos;
#if 0
    minPos = (minPos.z < baseHeight && minPos.z != maxPos.z) ?
        intersect(minPos, maxPos - minPos, vec3(0, 0, 1), baseHeight) :
        minPos;
    maxPos = maxPos.z < baseHeight ? minPos : maxPos;
#endif
    float minHeight = minPos.z;
    float maxHeight = maxPos.z;
    
    minHeight = max(minHeight, baseHeight);
    maxHeight = max(maxHeight, baseHeight);

    float deltaHeight = maxHeight - minHeight;

    float avgDensityScale = deltaHeight != 0.f ?
                (
                    FOG_DENSITY_SCALE_ANTIDERIVATIVE(maxHeight - baseHeight, falloff) -
                    FOG_DENSITY_SCALE_ANTIDERIVATIVE(minHeight - baseHeight, falloff)
                )
            / deltaHeight :
        FOG_DENSITY_SCALE(minHeight - baseHeight, falloff);


    return length(minPos - maxPos) * avgDensityScale * density;
}



#define MIST_EXPONENT 2

#define MIST_DENSITY_SCALE(h, top) \
    (pow((h) - (top), MIST_EXPONENT) / pow(top, MIST_EXPONENT))

#define MIST_DENSITY_SCALE_ANTIDERIVATIVE(h, top) \
    (pow((h) - (top), MIST_EXPONENT + 1) / (pow(top, MIST_EXPONENT) * (MIST_EXPONENT + 1)))

float calcMistDensity(float baseHeight, float layerTop, float densityFactor, vec3 cameraPos, vec3 worldPos)
{
    vec3 minPos = cameraPos.z < worldPos.z ? cameraPos : worldPos;
    vec3 maxPos = cameraPos.z < worldPos.z ? worldPos : cameraPos;

    // only the distance below the fog layer top is relevant
    maxPos = (maxPos.z > layerTop && minPos.z != maxPos.z) ?
            intersect(minPos, maxPos - minPos, vec3(0, 0, 1), layerTop) :
            maxPos;
    minPos = minPos.z > layerTop ? maxPos : minPos;

    minPos = (minPos.z < baseHeight && minPos.z != maxPos.z) ?
        intersect(minPos, maxPos - minPos, vec3(0, 0, 1), baseHeight) :
        minPos;

    maxPos = maxPos.z < baseHeight ? minPos : maxPos;

    float minHeight = minPos.z;
    float maxHeight = maxPos.z;

    minHeight = clamp(minHeight, baseHeight, layerTop);
    maxHeight = clamp(maxHeight, baseHeight, layerTop);

    float deltaHeight = maxHeight - minHeight;

    float avgDensityScale = deltaHeight != 0.f ?
                (MIST_DENSITY_SCALE_ANTIDERIVATIVE(maxHeight - baseHeight, layerTop - baseHeight) -
                    MIST_DENSITY_SCALE_ANTIDERIVATIVE(minHeight - baseHeight, layerTop - baseHeight))
            / deltaHeight :
        MIST_DENSITY_SCALE(minHeight - baseHeight, layerTop - baseHeight);

    avgDensityScale = abs(avgDensityScale);

    return length(minPos - maxPos) * avgDensityScale * densityFactor;
}



float calcFogDensity(vec3 cameraPos, vec3 worldPos)
{
    float density = 0;

#if 1
    // high athmospherical haze
    float visibility = 5000 / 100.0;
//     density += calcConstantFogLayerValue(-99000, 0, FOG_LAYER_DENSITY(visibility), cameraPos, worldPos);
    density += calcFogLayerValue(
            0.0,
            FOG_LAYER_FALLOFF(200 / 100.0),
//             FOG_LAYER_FALLOFF(1200 / 100.0),
            FOG_LAYER_DENSITY(visibility),
            cameraPos.xyz,
            worldPos.xyz);
#endif


//     density += calcConstantFogLayerValue(-99000, 0, FOG_LAYER_DENSITY(1600*100), cameraPos, worldPos);
//     density += calcMistDensity(0, 100*100, FOG_LAYER_DENSITY(1600*100), cameraPos, worldPos);

    density += calcMistDensity(0, 100*100, FOG_LAYER_DENSITY(600*100), cameraPos, worldPos);
//     density += calcMistDensity(0, 10*100, FOG_LAYER_DENSITY(140*100), cameraPos, worldPos);
//     density += calcMistDensity(0, 3*100, FOG_LAYER_DENSITY(60*100), cameraPos, worldPos);

    return density;
}

#include "world.glsl"

void main(void)
{
    vec3 sunPos = world.sunDirection * 500*1000*100;
    float sunVisibility = clamp(exp(-calcFogDensity(world.cameraPos, sunPos)) * 20, 0, 1);
    float sunBrightness = clamp(world.sunDirection.z + 0.5, 0, 1);
    sunBrightness *= clamp(dot(gl_LightSource[0].diffuse.xyz, vec3(0.2126, 0.7152, 0.0722)), 0, 1);
    float scale = 2 * clamp(sunVisibility - 0.5, 0, 1) * sunBrightness;

    vec4 vertexScaled = gl_Vertex;
    vertexScaled.xyz *= scale;
//     clamp(sunVisibility * 1, 0, 1);

    gl_Position = gl_ModelViewProjectionMatrix * vertexScaled;
    
//     gl_Position.xyz = (gl_ModelViewProjectionMatrix * (gl_Vertex * 0.3)).xyz;
//     gl_Position.a = (gl_ModelViewProjectionMatrix * (gl_Vertex * 0.3)).a;
    
//     gl_Position.z = 0;

//     gl_Position *= ;

    depth = gl_Position.z;
    passViewPos = (gl_ModelViewMatrix * (gl_Vertex * sunVisibility)).xyz;
    diffuseMapUV = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
}
