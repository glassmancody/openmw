#version 120

// varying vec3 passWorldPos;
varying vec3 passViewPos;
uniform mat4 osg_ViewMatrixInverse;


#include "world.glsl"
#include "sunlight.glsl"


void applyFog(vec3 cameraPos, vec3 worldPos);
vec3 intersect(vec3 lineP, vec3 lineN, vec3 planeN, float planeD);


void main(void)
{
    gl_FragData[0] = vec4(0,0,0,1);

//     gl_FragData[0] = vec4(1,0,0,1);

// #if 0
//     vec3 worldPos = passWorldPos;
// 
//     vec3 objPos = worldPos - world.cameraPos;
//     objPos *= 15000;
//     worldPos = objPos + world.cameraPos;
// //     vec3(cameraPos.x, cameraPos.y, 0);
// 
// 
// #if 0
//     gl_FragData[0].xyz = vec3(0.5, 0.5, 0);
//     if (worldPos.z > 0.0)
//         gl_FragData[0].xyz = vec3(0,0,1);
// #endif
// 
// 
// #if 0
//     gl_FragData[0].xyz = (cameraPos.z >= 0.0) ?
//         applyFog(cameraPos.xyz, worldPos.xyz, sunDirection, gl_FragData[0].xyz) :
//         gl_Fog.color.xyz;
// //     gl_FragData[0].xyz = applyFog(cameraPos.xyz, worldPos.xyz, sunDirection, gl_FragData[0].xyz);
// #endif
// #endif


    vec3 worldPos = (osg_ViewMatrixInverse * vec4(passViewPos.xyz, 1)).xyz;

    vec3 viewDir = -normalize(world.cameraPos - worldPos);

    vec3 objPos = worldPos - world.cameraPos;


    vec3 dir = normalize(objPos);
    objPos = dir * 500 * 1000 * 100;
    
//     objPos *= 150000;
//     objPos *= 5150000;
//     objPos *= 2150000;
    worldPos = objPos + world.cameraPos;


#if 0
    if (viewDir.z < 0) {
        // if we look below the horizon we see water
        worldPos = intersect(world.cameraPos, viewDir, vec3(0, 0, 1), world.waterLevel);
        gl_FragData[0] = vec4(0.090195, 0.115685, 0.12745, 1); // water color
//         gl_FragData[0] = vec4(1, 0.115685, 0.12745, 1); // water color
    }
#endif


// #if 0
//     if (true) {
// 
//         float d = length(world.cameraPos - worldPos);
// 
//         gl_FragData[0].r = 1;
// 
//     //     if (d > 100*1000*100)
//     //         gl_FragData[0].xyz = vec3(0.3, 0.3, 1);
//     // 
//     //     if (d > 200*1000*100)
//     //         gl_FragData[0].xyz = vec3(0.7, 0.7, 1);
//     // 
//     //     if (d > 500*1000*100)
//     //         gl_FragData[0].xyz = vec3(0, 0, 0);
// 
//         if (d > 1*1000*1000*100)
//             gl_FragData[0].xyz = vec3(0, 1, 0);
// 
//     //     if (d > 2000*1000*100)
//     //         gl_FragData[0].xyz = vec3(0, 0, 0);
// 
//         if (d > 5*1000*1000*100)
//             gl_FragData[0].xyz = vec3(0, 1, 1);
//             
//         if (d > 10*1000*1000*100)
//             gl_FragData[0].xyz = vec3(1, 0, 0);
// 
//         if (d > 20*1000*1000*100)
//             gl_FragData[0].xyz = vec3(0, 0, 0);
// 
//     }
// 
// 
// 
// //     if (worldPos.z > 0.1)
// //         gl_FragData[0].xyz = vec3(1,0,0);
// 
// 
// 
// #if 0
//     if (worldPos.z > 10*1000*100)
//         gl_FragData[0].xyz = vec3(0,1,0);
// 
//     if (worldPos.z > 20*1000*100)
//         gl_FragData[0].xyz = vec3(0,0,1);
// 
//     if (worldPos.z > 30*1000*100)
//         gl_FragData[0].xyz = vec3(1,1,0);
//         
//     if (worldPos.z > 40*1000*100)
//         gl_FragData[0].xyz = vec3(1,0,1);
// 
//     if (worldPos.z > 100*1000*100)
//         gl_FragData[0].xyz = vec3(0,0,1);
// #endif
// 
// 
//         
// #endif

    applyFog(world.cameraPos, worldPos);


//     if (abs(world.cameraPos.z - passWorldPos.z) <= 0.5)
//         gl_FragData[0].xyz = vec3(0, 1, 0);

//         gl_FragData[0].xyz = vec3(1, 0, 1);

//         gl_FragData[0].xyz = vec3(0);
}
