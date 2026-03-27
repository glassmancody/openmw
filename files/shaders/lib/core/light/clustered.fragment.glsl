#version 430 compatibility

#include "lib/light/bindings.glsl"
#include "lib/light/util.glsl"

uniform float near;
uniform vec2 screenRes;
uniform vec3 gridSize;

vec3 turboColormap(float x)
{
    // show clipping
    if(x < 0.0)
        return vec3(0.0);
    else if(x > 1.0)
        return vec3(1.0);

    const vec4 kRedVec4   = vec4(0.13572138, 4.61539260, -42.66032258, 132.13108234);
    const vec4 kGreenVec4 = vec4(0.09140261, 2.19418839, 4.84296658, -14.18503333);
    const vec4 kBlueVec4  = vec4(0.10667330, 12.64194608, -60.58204836, 110.36276771);
    const vec2 kRedVec2   = vec2(-152.94239396, 59.28637943);
    const vec2 kGreenVec2 = vec2(4.27729857, 2.82956604);
    const vec2 kBlueVec2  = vec2(-89.90310912, 27.34824973);

    x = clamp(x, 0.0, 1.0);
    vec4 v4 = vec4(1.0, x, x * x, x * x * x);
    vec2 v2 = v4.zw * v4.z;
    return vec3(
        dot(v4, kRedVec4)    + dot(v2, kRedVec2),
        dot(v4, kGreenVec4)  + dot(v2, kGreenVec2),
        dot(v4, kBlueVec4)   + dot(v2, kBlueVec2)
    );
}


void doLighting(vec2 screenCoord, vec3 viewPos, vec3 viewNormal, float shininess, float shadowing, out vec3 diffuseLight, out vec3 ambientLight, out vec3 specularLight) {
    vec3 viewDir = normalize(viewPos);
    shininess = max(shininess, 1e-4);

    diffuseLight = vec3(0.0);
    ambientLight = vec3(0.0);
    specularLight = vec3(0.0);

    calcDirectionalLighting(sun, viewDir, viewNormal, shininess, diffuseLight, ambientLight, specularLight);

    diffuseLight *= shadowing;
    specularLight *= shadowing;

    int tileIndex = getClusterTileIndex(screenRes, gridSize, near, screenCoord, viewPos.z);

    LightGrid grid = lightGrid[tileIndex];

    // int zTile = int((log(abs(viewPos.z) / near) * int(gridSize.z)) / log(8192.0 / near));
    // vec3 colors[6] = vec3[](
    //     vec3(1.0, 0.0, 0.0),
    //     vec3(0.0, 1.0, 0.0),
    //     vec3(0.0, 0.0, 1.0),
    //     vec3(1.0, 1.0, 0.0),
    //     vec3(0.0, 1.0, 1.0),
    //     vec3(1.0, 0.0, 1.0)
    // );
    // gl_FragData[0].rgb = colors[zTile% 6u];

    // vec3 lightCountColor = turboColormap(grid.count / 375.);
    // gl_FragData[0].rgb = lightCountColor;
    // return;

    for (uint i = 0; i < grid.count; ++i)
    {
        uint lightIdx = lightIndexList[grid.offset + i];
        PointLight light = pointLight[lightIdx];

        calcPointLighting(light, viewDir, viewPos, viewNormal, shininess, diffuseLight, ambientLight, specularLight);
    }
}
