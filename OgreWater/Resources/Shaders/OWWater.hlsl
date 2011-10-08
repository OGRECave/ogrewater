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

uniform sampler2D reflectionTexture : register(s0);
uniform sampler2D refractionTexture : register(s1);
uniform sampler2D reflectionDepthTexture : register(s2);
uniform sampler2D refractionDepthTexture : register(s3);
uniform sampler2D normalTexture : register(s4);

uniform float waterFogDensity = 10.0;
uniform float4 waterColor = float4(0.0, 1.0, 0.0, 1.0);

struct VS_INPUT
{
	float4 position : POSITION;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT;
	float2 texCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position			: POSITION;
	float3 normal			: NORMAL;
	float2 texCoord			: TEXCOORD0;
	float3 viewDirection	: TEXCOORD1;
	float3 viewDirectionTS	: TEXCOORD2;
	float3 lightDirection	: TEXCOORD3;
	float3 lightDirectionTS	: TEXCOORD4;
	float3 positionWS		: TEXCOORD5;
};

struct PS_INPUT
{
	float2 screenPos		: VPOS;
	float3 normal			: NORMAL;
	float2 texCoord			: TEXCOORD0;
	float3 viewDirection	: TEXCOORD1;
	float3 viewDirectionTS	: TEXCOORD2;
	float3 lightDirection	: TEXCOORD3;
	float3 lightDirectionTS	: TEXCOORD4;
	float3 positionWS		: TEXCOORD5;
};

struct PS_OUTPUT
{
	float4 color	: COLOR;
};

VS_OUTPUT main_vp(VS_INPUT input,
		uniform float4x4 world,
		uniform float4x4 worldViewProj,
		uniform float3 cameraPosition,
		uniform float3 lightDirection)
{
	VS_OUTPUT output;

	output.position = mul(worldViewProj, input.position);
	output.normal = input.normal;
	output.texCoord = input.texCoord;
	output.viewDirection = input.position - cameraPosition;
	output.lightDirection = lightDirection;

	float3x3 objectToTangentSpace;
	objectToTangentSpace[0] = input.tangent;
	objectToTangentSpace[1] = cross(input.tangent, input.normal);
	objectToTangentSpace[2] = input.normal;
	output.lightDirectionTS = mul(objectToTangentSpace, lightDirection);
	output.viewDirectionTS = mul(objectToTangentSpace, output.viewDirection);

	output.positionWS = mul(world, input.position);

	return output;
}

PS_OUTPUT main_fp(PS_INPUT input,
		uniform float4 cameraPosition,
		uniform float4 viewportSize,
		uniform float time,
		uniform float4 waterFogColor,
		uniform float4 materialVariables)
{
	PS_OUTPUT output;

	bool aboveSurface = (cameraPosition.y >= 200);

	float xCoord = (input.screenPos.x + 0.5) / viewportSize.x;
	float yCoord = (input.screenPos.y + 0.5) / viewportSize.y;

	float3 normalizedViewDirection = normalize(input.viewDirection);
	float3 normalizedViewDirectionTS = normalize(input.viewDirectionTS);
	
	float3 normal1 = normalize(2 * tex2D(normalTexture, 10 * float2(input.texCoord.x + 0.5 * time, input.texCoord.y)) - 1.0);
	float3 normal2 = normalize(2 * tex2D(normalTexture, 10 * float2(input.texCoord.x, input.texCoord.y + 0.5 * time)) - 1.0);
	float3 normal3 = normalize(2 * tex2D(normalTexture, 10 * float2(input.texCoord.x - 0.5 * time, input.texCoord.y + 0.0005)) - 1.0);
	float3 normal4 = normalize(2 * tex2D(normalTexture, 10 * float2(input.texCoord.x + 0.005, input.texCoord.y - 0.5 * time)) - 1.0);
	float3 normal = normalize(normal1 + normal2 + normal3 + normal4);

	//float3 normal = normalize(input.normal);

	float reflection = tex2D(reflectionDepthTexture, float2(xCoord, yCoord)).r;
	float reflectionDepth = reflection - length(input.positionWS - cameraPosition);
	if (reflection == 0.0 || reflectionDepth > 1000.0)
	{
		reflectionDepth = 1000.0;
	}

	float refraction = tex2D(refractionDepthTexture, float2(xCoord, yCoord)).r;
	float refractionDepth = refraction - length(input.positionWS - cameraPosition);
	if (refraction == 0.0)
	{
		refractionDepth = 1000.0;
	}

	float2 reflectionTexCoord = float2(xCoord, yCoord) + (normal.xy * reflectionDepth * materialVariables.x) / length(input.positionWS - cameraPosition);
	float4 reflectionColor = tex2D(reflectionTexture, reflectionTexCoord);
	if (reflectionColor.a == 0.0)
	{
		reflectionColor = tex2D(reflectionTexture, float2(xCoord, yCoord));
	}
	else
	{
		reflection = tex2D(reflectionDepthTexture, reflectionTexCoord).r;
	}

	float2 refractionTexCoord = float2(xCoord, yCoord) + normal.xy * refractionDepth * materialVariables.y;
	float4 refractionColor = tex2D(refractionTexture, refractionTexCoord);
	if (refractionColor.a == 0.0)
	{
		refractionColor = tex2D(refractionTexture, float2(xCoord, yCoord));
	}
	else
	{
		refraction = tex2D(refractionDepthTexture, refractionTexCoord).r;
	}

	float specular = 0;

	float3 light = normalize(input.lightDirection);
	if (aboveSurface)
	{
		float3 half = -normalize(light + normalizedViewDirection);
		specular = pow(saturate(dot(normal, half)), 250);
	}
	else if (!aboveSurface && refraction <= 0.0)
	{
		// If below surface, mirror light direction in z plane
		light.z = -light.z;
		float3 half = -normalize(light + normalizedViewDirection);
		specular = pow(saturate(dot(-normal, half)), 250);
	}

	float phi1 = 0.0;
	
	if (aboveSurface)
	{
		phi1 = acos(dot(-normalizedViewDirection, normal));
	}
	else
	{
		phi1 = acos(dot(-normalizedViewDirection, -normal));
	}

	float phi2 = asin((1/1.33) * sin(phi1));

	float R = saturate(
		pow(
			(sin(phi1 - phi2) / sin(phi1 + phi2)),
			2) +
		pow(
			(tan(phi1 - phi2) / tan(phi1 + phi2)),
			2));
	float T = 1.0 - R;

	if (aboveSurface)
	{
		// Above surface, fog the refraction color
		float f = 1 / exp(pow(refractionDepth * materialVariables.z, 2));
		refractionColor = f * refractionColor + (1-f) * waterFogColor;
	}
	else
	{
		// Below the surface, fog the reflection color
		float f = 1 / exp(pow(reflectionDepth * materialVariables.z, 2));
		reflectionColor = f * reflectionColor + (1-f) * waterFogColor;
	}

	output.color = R * reflectionColor + T * refractionColor + specular * float4(1.0, 1.0, 1.0, 1.0);

	return output;
}