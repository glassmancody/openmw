varying float minAtmosphereBrightness;
varying float maxAtmosphereBrightness;
varying float ambientBrightness;
varying vec3 ambientLight;
varying vec3 sunLight;
varying float sunBrightness;

vec3 applyPerception(vec3 color)
{
    return pow(color, vec3(1.7, 1.45, 1.0));
}
