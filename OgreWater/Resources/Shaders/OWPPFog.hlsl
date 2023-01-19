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

#include <OgreUnifiedShader.h>

SAMPLER2D(Scene, 0);
SAMPLER2D(RT, 1);
SAMPLER2D(SceneDepthTexture, 2);

#include "OWPPEffect.hlsl"

PS_OUTPUT main_fp(PS_INPUT input,
		uniform float4 waterFogColor,
		uniform float4 materialVariables)
{
	PS_OUTPUT output;

	float depth = tex2D(SceneDepthTexture, input.texCoord).r;
	float3 sceneColor = tex2D(Scene, input.texCoord).rgb;
	float3 blurredColor = tex2D(RT, input.texCoord).rgb;

	float f = min(saturate(1 / exp(pow(depth * materialVariables.z, 2))), 0.5);
	float blur = saturate(1 / exp(pow(depth * materialVariables.w, 2)));

	sceneColor = blur * sceneColor + (1-blur) * blurredColor;

	output.color = float4(f * sceneColor + (1-f) * waterFogColor.rgb, 1.0);
	return output;
}