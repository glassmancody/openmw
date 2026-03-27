#version 430 core

#include "lib/light/bindings.glsl"
#include "lib/light/util.glsl"

uniform float near;
uniform vec2 screenRes;
uniform vec3 gridSize;

void doLighting(vec2 screenCoord, vec3 viewPos, vec3 viewNormal, float shininess, out vec3 diffuseLight, out vec3 ambientLight, out vec3 specularLight, out vec3 shadowDiffuse, out vec3 shadowSpecular) {
    vec3 viewDir = normalize(viewPos);
    shininess = max(shininess, 1e-4);

    diffuseLight = vec3(0.0);
    ambientLight = vec3(0.0);
    specularLight = vec3(0.0);
    shadowDiffuse = vec3(0.0);

    calcDirectionalLighting(sun, viewDir, viewNormal, shininess, shadowDiffuse, ambientLight, shadowSpecular);

    int tileIndex = getClusterTileIndex(screenRes, gridSize, near, screenCoord, viewPos.z);

    LightGrid grid = lightGrid[tileIndex];

    for (uint i = 0; i < grid.count; ++i)
    {
        uint lightIdx = lightIndexList[grid.offset + i];
        PointLight light = pointLight[lightIdx];

        calcPointLighting(light, viewDir, viewPos, viewNormal, shininess, diffuseLight, ambientLight, specularLight);
    }
}
