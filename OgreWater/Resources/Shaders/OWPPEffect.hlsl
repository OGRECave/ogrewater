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

struct VS_INPUT
{
	float4 position : POSITION;
};

struct VS_OUTPUT
{
	float4 position			: POSITION;
	float2 texCoord			: TEXCOORD0;
};

struct PS_INPUT
{
	float4 position			: POSITION;
	float2 texCoord			: TEXCOORD0;
};

struct PS_OUTPUT
{
	float4 color	: COLOR;
};

VS_OUTPUT main_vp(VS_INPUT input,
		uniform float4x4 worldViewProj)
{
	VS_OUTPUT output;

    // Use standardise transform, so work accord with render system specific (RS depth, requires texture flipping, etc)
    output.position = mul(worldViewProj, input.position);

    // The input positions adjusted by texel offsets, so clean up inaccuracies
    input.position.xy = sign(input.position.xy);

    // Convert to image-space
    output.texCoord = (float2(input.position.x, -input.position.y) + 1.0f) * 0.5f;

	return output;
}