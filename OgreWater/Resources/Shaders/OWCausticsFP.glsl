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

SAMPLER2D(normalTexture, 0);
OGRE_UNIFORMS(
uniform float time;
uniform vec4 waterData; // x: waterHeight, y: waterDataY (scale/threshold)
)

const float PI = 3.1415926535897932384626433832795;

MAIN_PARAMETERS
IN(vec3 vNormal, NORMAL);
IN(vec3 vPositionWS, TEXCOORD0);
IN(vec3 vLightDirection, TEXCOORD1);
MAIN_DECLARATION
{
	float waterHeight = waterData.x - vPositionWS.y;

	// Caustics highlight calculation
	float c = (waterHeight - waterData.y) / waterData.y;
	c = clamp(c, -1.0, 1.0);
	float causticsHighlight = (1.0 + cos(PI * c)) * 0.5;

	// Projected texture coordinates for caustics
	float t = dot(vec3(0.0, 1.0, 0.0), -vLightDirection);
	vec2 texCoord = 0.001 * (vPositionWS.xz + ((waterHeight / t) * (-vLightDirection) - waterHeight * vec3(0.0, 1.0, 0.0)).xz);

	// Sample and blend normals (4 samples with animation)
	vec3 normal = vec3(0.0);
	normal += normalize(2.0 * texture(normalTexture, vec2(texCoord.x, texCoord.y - 5.0 * time)).rgb - 1.0);
	normal += normalize(2.0 * texture(normalTexture, vec2(texCoord.y, 1.0 - texCoord.x - 5.0 * time)).rgb - 1.0);
	normal += normalize(2.0 * texture(normalTexture, vec2(1.0 - texCoord.x, 1.0 - texCoord.y - 5.0 * time)).rgb - 1.0);
	normal += normalize(2.0 * texture(normalTexture, vec2(1.0 - texCoord.y, texCoord.x - 5.0 * time)).rgb - 1.0);
	normal.z *= 0.1; // Reduce z-component influence
	normal = normalize(normal);

	// Light intensity calculation (3 specular highlights)
	float intensity = 0.1 * pow(max(0.0, dot(normal.xzy, -vLightDirection)), 10.0);
	intensity += 0.1 * pow(max(0.0, dot(normal.xzy, -vLightDirection)), 100.0);
	intensity += 0.1 * pow(max(0.0, dot(normal.xzy, -vLightDirection)), 1000.0);

	// Apply vertex normal influence if present
	if (length(vNormal) != 0.0) {
		intensity *= max(0.0, dot(vNormal, -vLightDirection));
	}

	// Final color (alpha set to 0.0 as in original)
	gl_FragColor = causticsHighlight * intensity * vec4(1.0, 1.0, 1.0, 0.0);
}

