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
SAMPLER2D(tex, 0);

struct VS_INPUT
{
	float4 position : POSITION;
	float2 texCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position		: POSITION;
	float2 texCoord		: TEXCOORD0;
	float3 positionWS	: TEXCOORD1;
};

struct PS_INPUT
{
	float2 texCoord		: TEXCOORD0;
	float3 positionWS	: TEXCOORD1;
};

struct PS_OUTPUT
{
	float4 depth	: COLOR;
};

VS_OUTPUT main_vp(VS_INPUT input,
		uniform float4x4 world,
		uniform float4x4 worldViewProj)
{
	VS_OUTPUT output;

	output.position = mul(worldViewProj, input.position);
	output.positionWS = mul(world, input.position);
	output.texCoord = input.texCoord;

	return output;
}

PS_OUTPUT main_fp(PS_INPUT input,
		uniform float3 cameraPosition)
{
	PS_OUTPUT output;

	output.depth = float4(length(input.positionWS - cameraPosition), 0.0, 0.0, tex2D(tex, input.texCoord).a);

	return output;
}