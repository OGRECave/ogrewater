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

uniform sampler2D normalTexture : register(s0);

static const float PI = 3.1415926535897932384626433832795;

struct VS_INPUT
{
	float4 position		: POSITION;
	float3 normal		: NORMAL;
};

struct VS_OUTPUT
{
	float4 position			: POSITION;
	float3 normal			: NORMAL;
	float3 positionWS		: TEXCOORD0;
	float3 lightDirection	: TEXCOORD1;
};

struct PS_INPUT
{
	float3 normal			: NORMAL;
	float3 positionWS		: TEXCOORD0;
	float3 lightDirection	: TEXCOORD1;
};

struct PS_OUTPUT
{
	float4 color	: COLOR;
};

VS_OUTPUT main_vp(VS_INPUT input,
		uniform float4x4 world,
		uniform float4x4 worldViewProj,
		uniform float3 lightDirection)
{
	VS_OUTPUT output;

	output.position = mul(worldViewProj, input.position);
	output.positionWS = mul(world, input.position);
	output.normal = input.normal;
	output.lightDirection = lightDirection;

	return output;
}

PS_OUTPUT main_fp(PS_INPUT input,
		uniform float time,
		uniform float4 waterData)
{
	PS_OUTPUT output;

	float waterHeight = waterData.x - input.positionWS.y;

	float c = (waterHeight - waterData.y) / waterData.y;
	c = clamp(c, -1.0, 1.0);
	float causticsHighlight = (1.0 + cos(PI * c)) / 2.0;

	float3 normal = float3(0.0, 0.0, 0.0);

	float3 t = dot(float3(0.0, 1.0, 0.0), -input.lightDirection);
	float2 texCoord = 0.001 * (input.positionWS.xz + ((waterHeight / t) * (-input.lightDirection) - waterHeight * float3(0.0, 1.0, 0.0)).xz);

	normal = normalize(2 * tex2D(normalTexture, float2(texCoord.x, texCoord.y - 5 * time)) - 1.0);
	normal += normalize(2 * tex2D(normalTexture, float2(texCoord.y, 1.0 - texCoord.x - 5 * time)) - 1.0);
	normal += normalize(2 * tex2D(normalTexture, float2(1.0 - texCoord.x, 1.0 - texCoord.y - 5 * time)) - 1.0);
	normal += normalize(2 * tex2D(normalTexture, float2(1.0 - texCoord.y, texCoord.x - 5 * time)) - 1.0);
	normal.z *= 0.1;

	normal = normalize(normal);
	float intensity = 0.1 * pow(saturate(dot(normal.xzy, -input.lightDirection)), 10.0);
	intensity += 0.1 * pow(saturate(dot(normal.xzy, -input.lightDirection)), 100.0);
	intensity += 0.1 * pow(saturate(dot(normal.xzy, -input.lightDirection)), 1000.0);

	if (length(input.normal) != 0.0)
	{
		intensity *= saturate(dot(input.normal, -input.lightDirection));
	}

	output.color = causticsHighlight * intensity * float4(1.0, 1.0, 1.0, 0.0);

	return output;
}