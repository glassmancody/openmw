#version 120

uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;

#include "world.glsl"

void main(void)
{
    gl_FragData[0] = texture2D(diffuseMap, diffuseMapUV);// * gl_FrontMaterial.emission;

    gl_FragData[0].rgb += gl_LightSource[0].diffuse.rgb;
    gl_FragData[0].rgb *= 0.5;

    vec3 sunPos = world.sunDirection * 500*1000*100;
//     float sunVisibility = clamp(exp(-calcFogDensity(world.cameraPos, sunPos)) * 20, 0, 1);
    float sunBrightness = clamp(world.sunDirection.z + 0.5, 0, 1);
    sunBrightness *= clamp(dot(gl_LightSource[0].diffuse.xyz, vec3(0.2126, 0.7152, 0.0722)), 0, 1);
//     float scale = 2 * clamp(sunVisibility - 0.5, 0, 1) * sunBrightness;

//     float a = scale;//clamp(2 * scale, 0, 1);

//     gl_FragData[0].a = min(
//         gl_FragData[0].a * gl_FrontMaterial.diffuse.a,
//         gl_FragData[0].a * pow(clamp(sunVisibility * 3, 0, 1), 2));

//     gl_FragData[0].a *= pow(clamp(sunVisibility * 3, 0, 1), 2);
//     gl_FragData[0].a *= clamp(sunVisibility * 4, 0, 1);

//     gl_FragData[0].a *= a;

//     gl_FragData[0].a = 0;
}
