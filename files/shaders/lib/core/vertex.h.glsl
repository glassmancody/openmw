@link "lib/core/vertex.glsl" if !@useOVR_multiview
@link "lib/core/vertex_multiview.glsl" if @useOVR_multiview

vec4 modelToClip(vec4 pos);
vec4 modelToView(vec4 pos);
vec4 viewToClip(vec4 pos);
vec2 clipToScreen(vec4 pos);

@link "lib/core/light/clustered.vertex.glsl" if @lightingMethodClustered
@link "lib/core/light/legacy.vertex.glsl" if @lightingMethodPerObjectUniform

void doLighting(vec2 screenCoord, vec3 viewPos, vec3 viewNormal, float shininess, out vec3 diffuseLight, out vec3 ambientLight, out vec3 specularLight, out vec3 shadowDiffuse, out vec3 shadowSpecular);
