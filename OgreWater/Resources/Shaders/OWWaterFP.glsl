/*
Copyright (c) 2011 Anders Lingfors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
#include <OgreUnifiedShader.h>

SAMPLER2D(reflectionTexture, 0);
SAMPLER2D(refractionTexture, 1);
SAMPLER2D(reflectionDepthTexture, 2);
SAMPLER2D(refractionDepthTexture, 3);
SAMPLER2D(normalTexture, 4);

OGRE_UNIFORMS(
uniform vec4 cameraPosition;
uniform vec4 viewportSize;
uniform float time;
uniform vec4 waterFogColor;
uniform vec4 materialVariables;
)

MAIN_PARAMETERS
// IN(vec4 screenPos, VPOS)
IN(vec3 vsnormal, NORMAL)
IN(vec3 positionWS, TEXCOORD0)
IN(vec3 viewDirection, TEXCOORD1)
IN(vec3 viewDirectionTS, TEXCOORD2)
IN(vec3 olightDirection, TEXCOORD3)
IN(vec3 lightDirectionTS, TEXCOORD4)

MAIN_DECLARATION
{
	float waterHeight = waterFogColor.a;
	vec4 _waterFogColor = waterFogColor;
	_waterFogColor.a = 1.0;
	bool aboveSurface = (cameraPosition.y >= waterHeight);

	// Screen-space UVs (GLSL's gl_FragCoord is already in window coords)
	vec2 screenUV = (gl_FragCoord.xy + 0.5) / viewportSize.xy;

	vec3 normalizedViewDirection = normalize(viewDirection);
	vec3 normalizedViewDirectionTS = normalize(viewDirectionTS);

	// Sample and blend normals
	vec2 texCoord = 0.001 * positionWS.xz;
	vec3 normal = vec3_splat(0.0);
	normal += normalize(2.0 * texture2D(normalTexture, vec2(texCoord.x, texCoord.y - 5.0 * time)).rgb - 1.0);
	normal += normalize(2.0 * texture2D(normalTexture, vec2(texCoord.y, 1.0 - texCoord.x - 5.0 * time)).rgb - 1.0);
	normal += normalize(2.0 * texture2D(normalTexture, vec2(1.0 - texCoord.x, 1.0 - texCoord.y - 5.0 * time)).rgb - 1.0);
	normal += normalize(2.0 * texture2D(normalTexture, vec2(1.0 - texCoord.y, texCoord.x - 5.0 * time)).rgb - 1.0);
	normal = normalize(normal);

	// Depth calculations
	float reflectionDepth = texture2D(reflectionDepthTexture, screenUV).r - length(positionWS - cameraPosition.xyz);
	reflectionDepth = (reflectionDepth <= 0.0 || reflectionDepth > 500.0) ? 500.0 : reflectionDepth;

	float refractionDepth = texture2D(refractionDepthTexture, screenUV).r - length(positionWS - cameraPosition.xyz);
	refractionDepth = (refractionDepth <= 0.0 || refractionDepth > 500.0) ? 500.0 : refractionDepth;

	// Distorted texture lookups
	vec2 reflectionTexCoord = screenUV + (normal.xy * reflectionDepth * materialVariables.x) / length(positionWS - cameraPosition.xyz);
	vec4 reflectionColor = texture2D(reflectionTexture, reflectionTexCoord);
	if (reflectionColor.a == 0.0) {
		reflectionColor = texture2D(reflectionTexture, screenUV);
	}

	vec2 refractionTexCoord = screenUV + normal.xy * refractionDepth * materialVariables.y;
	vec4 refractionColor = texture2D(refractionTexture, refractionTexCoord);
	if (refractionColor.a == 0.0) {
		refractionColor = texture2D(refractionTexture, screenUV);
	}

	// Specular highlights
	float specular = 0.0;
	vec3 light = normalize(olightDirection);
	if (aboveSurface && texture2D(reflectionDepthTexture, screenUV).r <= 0.0) {
		vec3 halfVec = -normalize(light + normalizedViewDirection);
		specular = pow(max(0.0, dot(normal, halfVec)), 100.0);
	} else if (!aboveSurface && texture2D(refractionDepthTexture, screenUV).r <= 0.0) {
		light.z = -light.z;
		vec3 halfVec = -normalize(light + normalizedViewDirection);
		specular = pow(max(0.0, dot(-normal, halfVec)), 100.0);
	}

	// Fresnel calculations
	float phi1, sinPhi2, phi2;
	if (aboveSurface) {
		phi1 = acos(dot(-normalizedViewDirection, normal));
		sinPhi2 = 1.0f;
		phi2 = asin((1.0/1.33) * sin(phi1));
	} else {
		phi1 = acos(dot(-normalizedViewDirection, -normal));
		sinPhi2 = 1.33 * sin(phi1);
		phi2 = asin(clamp(sinPhi2, -1.0, 1.0));
	}

	float R, T;
	if (!aboveSurface && sinPhi2 > 1.0) {
		R = 1.0;
		T = 0.0;
	} else {
		R = clamp(pow(sin(phi1 - phi2)/sin(phi1 + phi2), 2.0) + pow(tan(phi1 - phi2)/tan(phi1 + phi2), 2.0), 0.0, 1.0);
		T = 1.0 - R;
	}

	// Fog blending
	if (aboveSurface) {
		float f = 1.0 / exp(pow(refractionDepth * materialVariables.z, 2.0));
		refractionColor = mix(_waterFogColor, refractionColor, f);
	} else {
		float f = 1.0 / exp(pow((reflectionDepth + length(positionWS - cameraPosition.xyz)) * materialVariables.z, 2.0));
		reflectionColor = mix(_waterFogColor, reflectionColor, f);
	}

	gl_FragColor = R * reflectionColor + T * refractionColor + specular * vec4_splat(1.0);
}
