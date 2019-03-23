#version 120

varying float depth;

void applyFog()
{
    float fogValue = clamp((depth - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
    gl_FragData[0].xyz = mix(gl_FragData[0].xyz, gl_Fog.color.xyz, fogValue);
}
