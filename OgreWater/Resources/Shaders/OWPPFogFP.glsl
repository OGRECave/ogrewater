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

SAMPLER2D(Scene, 0);
SAMPLER2D(RT, 1);
SAMPLER2D(SceneDepthTexture, 2);

OGRE_UNIFORMS(
uniform vec4 waterFogColor;
uniform vec4 materialVariables; // z: fog density, w: blur intensity
)
MAIN_PARAMETERS
IN(vec2 vTexCoord, TEXCOORD0)
MAIN_DECLARATION
{
	// Sample textures
	float depth = texture2D(SceneDepthTexture, vTexCoord).r;
	vec3 sceneColor = texture2D(Scene, vTexCoord).rgb;
	vec3 blurredColor = texture2D(RT, vTexCoord).rgb;

	// Fog and blur calculations
	float f = min(exp(-pow(depth * materialVariables.z, 2.0)), 0.5);
	float blur = exp(-pow(depth * materialVariables.w, 2.0));

	// Blend scene and blurred colors
	sceneColor = mix(blurredColor, sceneColor, blur);

	// Final fog blending
	gl_FragColor = vec4(mix(waterFogColor.rgb, sceneColor, f), 1.0);
}

