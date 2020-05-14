// light settings
#define CFL const float
CFL sunexp = 2.0; // sun exposure
CFL sunbexp = 0.075; // sunbounce exposure
CFL pnightexp = 5.0; // point lights night exposure
CFL pdayexp = 0.25; // point lights day exposure
CFL aintexp = 0.5; // ambient interior exposure
CFL aoutexp = 0.2; // ambient exterior exposure

CFL daysky = 1.7; // day time sky lighting exposure
CFL dayoc = 0.9; // day time overcast sky exposure
CFL nightoc = 0.33; // night time overcast sky exposure

CFL dayskysun = 0.5; // sky lighting sun contribution

CFL vcoff = 0.1; // vertex coloring offset(no blacking)
CFL vcexp = 0.3;  // vertex coloring exposure
CFL emivnight = 0.5; // emissive night exposure
CFL emivday = 0.15; // emissive day exposure
CFL emnight = 1.0; // emissive map night exposure
CFL emday = 0.5; // emissive map day exposure
CFL ambmin = 0.02; // ambient min (prevents too dark cells)