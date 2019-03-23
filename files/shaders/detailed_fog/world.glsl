struct Weather
{
    vec3 ambientLight;
    vec3 sunLight;
    float cloudsHeight;
    float hazeDryness;
};

struct World
{
    float waterLevel;
    vec3 sunDirection;
    vec3 cameraPos;
    float sunHeight;
    int weather0;
    int weather1;
    float weatherBlend;
};

const Weather weatherTable[8] = Weather[8] (
    Weather(vec3(1), vec3(1), 8000*100.0, 0.075), // clear
    Weather(vec3(1), vec3(0.7), 500*100.0, 0.03), // cloudy
    Weather(vec3(0.8), vec3(0.0), 500*100.0, 0.0), // foggy
    Weather(vec3(0.8), vec3(0.0), 500*100.0, 0.0), // overcast
    Weather(vec3(0.7), vec3(0.0), 500*100.0, 0.0), // rain
    Weather(vec3(0.7), vec3(0.0), 500*100.0, 0.0), // thunderstorm
    Weather(vec3(0.8, 0.5, 0.2), vec3(0.0), 500*100.0, 0.0), // ashstorm
    Weather(vec3(0.8, 0.5, 0.2), vec3(0.0), 500*100.0, 0.0)); // blight


uniform World world = World(0.0, vec3(0,0,1), vec3(0), 0.0, 0, 0, 0.f);

varying vec3 passAmbientLight;
varying vec3 passSunLight;
varying float passHazeDryness;
varying float passCloudsHeight;
