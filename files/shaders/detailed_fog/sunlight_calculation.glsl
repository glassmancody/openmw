const float PI = 3.1415926535897932384626433832795;


float calcAmbientDarkness()
{
    const float TWILIGHT_DISTANCE_DEG = 40;
    const float TWILIGHT_DISTANCE_RAD = 2 * PI * (TWILIGHT_DISTANCE_DEG / 360);

    float sunsetDistanceRad = (world.sunHeight * PI);
    float sunsetDistanceCamera = sunsetDistanceRad * -1;
    sunsetDistanceCamera = max(0, sunsetDistanceCamera);

    return min(sunsetDistanceCamera / TWILIGHT_DISTANCE_RAD, 1);
}

float calcSunDarkness()
{
    const float TWILIGHT_DISTANCE_DEG = 10;
    const float TWILIGHT_DISTANCE_RAD = 2 * PI * (TWILIGHT_DISTANCE_DEG / 360);

    float sunsetDistanceRad = (world.sunHeight * PI) - (TWILIGHT_DISTANCE_RAD/2);
    float sunsetDistanceCamera = sunsetDistanceRad * -1;
    sunsetDistanceCamera = max(0, sunsetDistanceCamera);

    return sunsetDistanceCamera / TWILIGHT_DISTANCE_RAD;
}

float calcSunBrightness()
{
    return 1 - calcSunDarkness();
}

vec3 calcAtmosphereBrightness()
{
    float darkness = calcAmbientDarkness();

    float maxBrightness = 1 - darkness;
    float minBrightness = mix(maxBrightness, maxBrightness * 0.0, darkness);

    float ambientBrightness = mix(minBrightness * 0.3, minBrightness, 1 - exp(-6 * world.sunHeight));

    return vec3(minBrightness, maxBrightness, ambientBrightness);
}

vec3 calcSunLight()
{
    vec3 light = passSunLight * calcSunBrightness();

    float extinction = 1 - smoothstep(-0.1, 0.6, world.sunHeight);

    light *= vec3(1) - pow(vec3(extinction), vec3(8, 2, 1));

    return light;
}

void calcCurrentWeather()
{
    Weather weather0 = weatherTable[world.weather0];
    Weather weather1 = weatherTable[world.weather1];
    float blend = world.weatherBlend;

    passAmbientLight = mix(weather0.ambientLight, weather1.ambientLight, blend);
    passSunLight = mix(weather0.sunLight, weather1.sunLight, blend);
    passHazeDryness = mix(weather0.hazeDryness, weather1.hazeDryness, blend);
    passCloudsHeight = mix(weather0.cloudsHeight, weather1.cloudsHeight, blend);
}

void calcSunLightParams()
{
    calcCurrentWeather();
    vec3 atmosphereBrightness = calcAtmosphereBrightness();

    minAtmosphereBrightness = atmosphereBrightness.x;
    maxAtmosphereBrightness = atmosphereBrightness.y;
    ambientBrightness = atmosphereBrightness.z;

    ambientLight = applyPerception(ambientBrightness * passAmbientLight);
    sunLight = calcSunLight();
    sunBrightness = calcSunBrightness();
}
