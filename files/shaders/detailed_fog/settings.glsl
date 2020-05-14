// contrast of terrain and objects 
#define CONTRAST 0.78

// size of the clouds, try to stay below 1.0 
// note: do NOT use any modded sky meshes, and make sure custom textures
//       are compatible with vanilla sky meshes 
#define CLOUD_SCALE 0.5

//#define FOG_COLOUR vec3(0.7, 0.8, 0.9)
#define FOG_COLOUR vec3(0.549, 0.8157, 0.9725)

#define RAYLEIGH_COLOUR vec3(0.1, 0.2, 0.9)

#define SCATTER_DARK vec3(0.15, 0.75, 1.0)
#define SCATTER_BRIGHT vec3(0.55, 0.8, 1.0)

// tint atmosphere color 
#define ATMOSPHERE_TINT vec3(0.9686, 0.0471, 0.0471)

// between 0 and 1.0
// the closer to 1.0, the more intense the tinting is 
#define ATMOSPHERE_MIX 0.0

// make sure these values have a decimal point!
#define ATMOSPHERE_VISIBILITY 250000.0
#define ATMOSPHERE_FALLOFF 2500.0

// see world.glsl for individual weather settings 