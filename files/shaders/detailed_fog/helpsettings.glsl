// constrast for day and night
// night setting also affects interiors
const float conday = 0.86;
const float connight = 0.87;

// height fog, decrease for more fog
const float fogheight = 0.5;

// experimental cloud shadows, remove slashes to enable 
//#define cloudshadows

// debug lighting, disables textures
// #define lightdebug

// debug albedo, shows albedo
//#define debugtextures

//#define debugvertexcol
// * tweakables end * //


float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }
vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }
vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }
vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }

float invLerp( float a, float b, float v ) { return (v - a) / (b - a); }
vec3 invLerp( vec3 a, vec3 b, float v ) { return (v - a) / (b - a); }


const mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );

vec2 hash( vec2 p ) {
	p = vec2(dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)));
	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise( in vec2 p ) {
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;
	vec2 i = floor(p + (p.x+p.y)*K1);	
    vec2 a = p - i + (i.x+i.y)*K2;
    vec2 o = (a.x>a.y) ? vec2(1.0,0.0) : vec2(0.0,1.0); //vec2 of = 0.5 + 0.5*vec2(sign(a.x-a.y), sign(a.y-a.x));
    vec2 b = a - o + K2;
	vec2 c = a - 1.0 + 2.0*K2;
    vec3 h = max(0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
	vec3 n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
    return dot(n, vec3(70.0));	
}


float cshadow(in vec2 p, in float oTime) {

	
	float time = oTime * 0.02; // speed

	float f = 0.0;
    vec2 uv = p;
	uv *= 0.00005; // scale
    float weight = 0.7;
    for (int i=0; i<3; i++){
		f += weight*noise( uv );
        uv = m*uv + time;
		weight *= 0.6;
    }
	
	return f;

}

float CalFade (float fv)
{
	return saturate(fv * fv * 0.00000001);
}

float CalFog (float fv)
{
	return clamp(exp(-3.3 + 4.0 * fv) -0.2, 0.0, 1.0);
}
float CalFogU (float fv)
{
	return clamp(exp(-3.3 + 10.0 * fv), 0.0, 1.0);
}

float calHFog (float fv)
{
	return 0.8 * clamp(exp(-7.4 + 10.0 * fv) - 0.05, 0.0, 1.0);
}


float FogMerge(float a, float b)
{
	return max(a,0.29 * ((a - b) * (a - b) + 2.0) * (a + b));
}

vec3 Uncharted2ToneMapping(vec3 color)
{
	float A = 0.105;
	float B = 0.714;
	float C = 0.57;
	float D = 0.21;
	float E = 0.092;
	float F = 0.886;
	float W = 600.0;
	float exposure = 2.;
	color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	return color;
}


vec3 SpecialContrast(vec3 x, float suncon) 
{
	//x = pow(x, vec3(0.5 * 2.2));
	vec3 contrasted = x*x*x*(x*(x*6.0 - 15.0) + 10.0);
	x.rgb = mix(x.rgb, contrasted, suncon);
	return x;
}
