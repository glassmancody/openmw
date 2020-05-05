#version 130

#include "world.glsl"

vec3 applyPerception(vec3 color);

varying float minAtmosphereBrightness;
varying float maxAtmosphereBrightness;
varying vec3 ambientLight;
varying vec3 sunLight;
varying float sunBrightness;

uniform bool isSunFlash = false;
uniform bool isVisibleQuery = false;
uniform bool isWaterReflection = false;
uniform bool isWaterRefraction = false;
uniform bool isClouds = false;
uniform bool isAtmosphere = false;
uniform mat4 osg_ViewMatrixInverse;

uniform sampler2DRect sedimentColorMap;

varying vec3 passViewPos;

#define EXPONENTIAL_FOG_DENSITY_SCALE(height, falloff) \
    exp(-(falloff) * height)
#define EXPONENTIAL_FOG_DENSITY_SCALE_ANTIDERIVATIVE(height, falloff) \
    (-(1/falloff) * exp(-falloff * height))

#define EXPONENTIAL_FOG_LAYER_FALLOFF(height1) \
    (log(0.5) / -(height1))

#define FOG_DENSITY_SCALE(h, top) \
    (pow((h) - (top), 2) / pow(top, 2))

#define FOG_DENSITY_SCALE_ANTIDERIVATIVE(h, top) \
    (pow((h) - (top), 3) / (pow(top, 2) * 3))

const float PI = 3.1415926535897932384626433832795;

const float atmosphereVisibility = 250000.0;
const float atmosphereDensityFalloff = 2500.0;


float miePhase(vec3 view_dir, vec3 light_dir, float g)
{
  return (1.f - g*g) / (4*PI * pow(1.f + g*g - 2*g*dot(view_dir, light_dir), 1.5));
}

vec3 calcTransmittance(vec3 visibility, float dist)
{
  return exp(-5 * (dist / visibility));
}

float calcFogAmount(float density)
{
  return 1.f - exp(-density);
}

vec3 intersect(vec3 lineP,
               vec3 lineN,
               vec3 planeN,
               float  planeD)
{
    float distance = (planeD - dot(planeN, lineP)) / dot(lineN, planeN);
    return lineP + lineN * distance;
}

float calcExponentialFogLayerDistance(
    float baseHeight,
    float falloff,
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
#if 1
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
                    EXPONENTIAL_FOG_DENSITY_SCALE_ANTIDERIVATIVE(maxHeight - baseHeight, falloff) -
                    EXPONENTIAL_FOG_DENSITY_SCALE_ANTIDERIVATIVE(minHeight - baseHeight, falloff)
                )
            / deltaHeight :
        EXPONENTIAL_FOG_DENSITY_SCALE(minHeight - baseHeight, falloff);


    return length(minPos - maxPos) * avgDensityScale * 10000;
}

float calcExponentialFogLayerRelativeValue(
    float baseHeight,
    float falloff,
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
#if 1
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
                    EXPONENTIAL_FOG_DENSITY_SCALE_ANTIDERIVATIVE(maxHeight - baseHeight, falloff) -
                    EXPONENTIAL_FOG_DENSITY_SCALE_ANTIDERIVATIVE(minHeight - baseHeight, falloff)
                )
            / deltaHeight :
        EXPONENTIAL_FOG_DENSITY_SCALE(minHeight - baseHeight, falloff);

    return avgDensityScale;
}

float calcFogLayerDistance(float baseHeight, float layerTop, vec3 cameraPos, vec3 worldPos)
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
                (FOG_DENSITY_SCALE_ANTIDERIVATIVE(maxHeight - baseHeight, layerTop - baseHeight) -
                    FOG_DENSITY_SCALE_ANTIDERIVATIVE(minHeight - baseHeight, layerTop - baseHeight))
            / deltaHeight :
        FOG_DENSITY_SCALE(minHeight - baseHeight, layerTop - baseHeight);

    avgDensityScale = abs(avgDensityScale);

    return length(minPos - maxPos) * avgDensityScale;
}

float calcFogDistance(vec3 cameraPos, vec3 worldPos)
{
  float dist = 0;

#if 1

#if 0
  float visibility = 20 * 1000 * 100.0;

  float h1 = 300;

  dist += calcExponentialFogLayerDistance(
          0.0,
          EXPONENTIAL_FOG_LAYER_FALLOFF(h1 / 100.0),
          cameraPos.xyz,
          worldPos.xyz) / visibility;
#endif

//   dist += calcFogLayerDistance(0, 30*100, cameraPos, worldPos) / (300*100);
  dist += calcFogLayerDistance(0, 100*100, cameraPos, worldPos) / (700*100);
  dist += calcFogLayerDistance(0, 300*100, cameraPos, worldPos) / (1500*100);

#endif

    if (!isWaterReflection)
        dist = pow(dist, 1.7);

  dist *= 3;

  return dist;
}

float calcVisibility(vec3 cameraPos, vec3 worldPos)
{
  return 1 - calcFogAmount(calcFogDistance(cameraPos, worldPos));
}

float calcSunVisibility(vec3 cameraPos, vec3 worldPos)
{
  return 1 - calcFogAmount(0.1 * calcFogDistance(cameraPos, worldPos));
}


void applyFogInternal(vec3 cameraPos, vec3 worldPos)
{
  if (isWaterRefraction)
    return;

  vec3 inputColour = gl_FragData[0].xyz;

#if 0
    // for debugging: mark sun and horizon
   if (length(world.sunDirection - normalize(worldPos - cameraPos)) < 0.01)
   {
        gl_FragData[0].xyz = vec3(1,0,1);
        return;
   }
   if (abs(normalize(worldPos - cameraPos).z) < 0.0005)
   {
        if (int(gl_FragCoord.x) % 8 == 0) {
            gl_FragData[0].xyz = vec3(0,0.5,0);
            return;
        }
   }
#endif

#if 1
    float fogDistance = calcFogDistance(cameraPos, worldPos);
    float fogAmount = calcFogAmount(fogDistance);

///////////////////////////////////////////////////////////////////////////////////////////////////////


#if 1
    float highAtmosphereDistance = 6 * calcExponentialFogLayerDistance(
            0,
            EXPONENTIAL_FOG_LAYER_FALLOFF(atmosphereDensityFalloff / 100.0),
            cameraPos.xyz,
            worldPos.xyz)
            /
            (atmosphereVisibility * 100.0);

    float atmosphereDistance = highAtmosphereDistance;
    atmosphereDistance += fogDistance * passHazeDryness;

    float atmosphereOpacity = 1.0 - exp(-atmosphereDistance);
#endif

    vec3 rayleighInput = ambientLight;

    vec3 rayleighColor = vec3(0.1, 0.2, 0.9);

#if 1
    if (isAtmosphere)
    {
        mat2 rot;
        {
            vec2 rv = normalize(vec2(1, 0) - normalize(world.sunDirection.xy));
            rot[0] = vec2(rv.x, rv.y);
            rot[1] = vec2(-rv.y, rv.x);
        }
        vec2 rotatedPos = rot * (worldPos.xy - cameraPos.xy);
        rotatedPos *= -1;

        float relativeBrightness = clamp(clamp(rotatedPos.x/100000 + 500, 0, 1000) / 1000, 0, 1);
        relativeBrightness *= relativeBrightness;

        vec3 rayleighInputAtmosphere = vec3(1);

        rayleighInputAtmosphere *= mix(minAtmosphereBrightness, maxAtmosphereBrightness, relativeBrightness);
        rayleighInputAtmosphere = applyPerception(rayleighInputAtmosphere);

        rayleighInput = mix(rayleighInputAtmosphere, rayleighInput, atmosphereOpacity);
//         rayleighInput = rayleighInputAtmosphere;
//         rayleighInput = applyPerception(rayleighInput);
    }
#endif

#if 1
    atmosphereDistance = (atmosphereDistance - highAtmosphereDistance) * (length(rayleighInput)/length(vec3(1))) + highAtmosphereDistance;


    rayleighColor *= rayleighInput;

    float atmosphereHaze = 0.15;
    atmosphereHaze = 0.5;

    vec3 diffuseScatteringColorDark = vec3(0.15, 0.75, 1.0);
    vec3 diffuseScatteringColorBright = vec3(0.55, 0.8, 1.0);

    diffuseScatteringColorDark *= rayleighInput;
    diffuseScatteringColorBright *= rayleighInput;

    if (!isAtmosphere)
    {
        rayleighColor = mix(rayleighColor, diffuseScatteringColorDark, 0.4);
    }

#endif

#if 0
    // dim stars
    if (isAtmosphere) {
        vec3 viewDir = -normalize(cameraPos - worldPos);
        if (viewDir.z > 0) {
            // atmosphere opacity increases with brightness (stars become invisible)
            float starsBrightness = exp(-12 * length(rayleighColor));
            starsBrightness *= exp(-30 * fogBrightness * fogAmount);
            inputColour *= starsBrightness;
        }
//         float blueShift = exp(-8 * length(rayleighColor));
//         rayleighColor = mix(rayleighColor, rayleighColor * vec3(0.0, 1.0, 2.0), blueShift);
    }
#endif

/////////////////////////////////////////////////////////////////////////////7
#if 1
    vec3 mieColor = vec3(1);
    {
        float mieDensity = calcExponentialFogLayerRelativeValue(
                0,
                EXPONENTIAL_FOG_LAYER_FALLOFF(atmosphereDensityFalloff / 100.0),
                cameraPos.xyz,
                worldPos.xyz);

        float mieDistance = mieDensity * distance(cameraPos.xyz, worldPos.xyz);

        float mieAmount = 1.5 * miePhase(normalize(worldPos - cameraPos), world.sunDirection, 0.6);

        float vis = 100 * 1000 * 100;
        float strength = 1 - exp(-5 * (mieDistance / vis));
        float extinction = pow(1 - smoothstep(0.0, 0.35, world.sunHeight), 1);

        float x = 100;
        mieColor = vec3(1) * calcTransmittance(vec3(2500, 150, 50) * 1000 * x,  mieDistance);

        mieColor = mix(vec3(1.0), mieColor, extinction);
        mieColor *= strength;
        mieColor *= mieAmount;
        mieColor *= clamp(sunBrightness * 2, 0, 1);
    }
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////7

    // --- rayleigh ---
//     float diffuseScatteringAmount = 1 + atmosphereHaze * 10;
//     float diffuseScatteringAmount = 2.0;
    float diffuseScatteringAmount = 1.0;
    rayleighColor *= 1.0 - exp(-15.0 * atmosphereDistance);
    rayleighColor = mix(rayleighColor, diffuseScatteringColorDark, 1.0 - exp(-2 * diffuseScatteringAmount * atmosphereDistance));
    rayleighColor = mix(rayleighColor, diffuseScatteringColorBright, 1.0 - exp(-1 * diffuseScatteringAmount * atmosphereDistance));
    rayleighColor *= 1-gl_Fog.scale;
    inputColour *= 1 - atmosphereOpacity;
    inputColour = mix(inputColour, vec3(1), rayleighColor);

    // mie
    if (isAtmosphere) {
      inputColour = mix(inputColour, vec3(1), mieColor);
    }

    // fog
    vec3 fogColor = vec3(0.7, 0.8, 0.9);
    
    fogColor *= 1.1;
    
//     fogColor *= 1.2;
//     fogColor = clamp(fogColor, vec3(0), vec3(1));
//     fogColor *= vec3(1, 0.95, 1);
//     fogColor *= 0.9;
//     fogColor *= vec3(0.9, 0.9, 1);
    
    fogColor *= ambientLight;

    fogAmount *= 1-gl_Fog.scale;

    inputColour = mix(inputColour, fogColor, fogAmount);

#endif

    gl_FragData[0].xyz = inputColour;
}

#if 1
void applyFog(vec3 cameraPos, vec3 worldPos)
{
    bool cameraUnderWater = cameraPos.z < world.waterLevel;

    if (isWaterReflection) {
        cameraPos.z = (-1 * (cameraPos.z - world.waterLevel)) + world.waterLevel;
        worldPos.z = (-1 * (worldPos.z - world.waterLevel)) + world.waterLevel;

        if (cameraPos.z != world.waterLevel) {
            cameraPos = intersect(cameraPos, cameraPos - worldPos, vec3(0,0,1), world.waterLevel);
            cameraPos.z = world.waterLevel;
        }
    }

    if (isClouds) {
      vec3 viewDir = -1 * (cameraPos - worldPos);

      if (isWaterReflection)
        viewDir *= -1;

      if (viewDir.z == 0) {
          worldPos = normalize(viewDir) * atmosphereVisibility;
      }
      else if (viewDir.z < 0) {
          // if we look below the horizon we see water
          worldPos = intersect(cameraPos, viewDir, vec3(0, 0, 1), world.waterLevel);
          gl_FragData[0] = vec4(0.090195, 0.115685, 0.12745, 1); // water color
      }
      else
          worldPos = intersect(cameraPos, viewDir, vec3(0, 0, 1), passCloudsHeight);
    }

    applyFogInternal(cameraPos, worldPos);
}
#endif

void applyFog()
{
    vec3 worldPos = (osg_ViewMatrixInverse * vec4(passViewPos.xyz, 1)).xyz;
    vec3 cameraPos = world.cameraPos;

    applyFog(cameraPos, worldPos);
}
