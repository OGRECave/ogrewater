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

OGRE_UNIFORMS(
uniform mat4 world;
uniform mat4 worldViewProj;
uniform vec4 cameraPositionOS;
uniform vec3 lightDirection;
)
MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec3 normal, NORMAL)
IN(vec3 tangent, TANGENT)
OUT(vec3 vNormal, NORMAL)
OUT(vec3 positionWS, TEXCOORD0)
OUT(vec3 viewDirection, TEXCOORD1)
OUT(vec3 viewDirectionTS, TEXCOORD2)
OUT(vec3 olightDirection, TEXCOORD3)
OUT(vec3 lightDirectionTS, TEXCOORD4)

MAIN_DECLARATION
{
	gl_Position = mul(worldViewProj, vertex);
	positionWS = mul(world, vertex).xyz;
	vNormal = normal;
	viewDirection = vertex.xyz - cameraPositionOS.xyz;
	olightDirection = lightDirection;
	vec3 T = tangent;
	vec3 B = cross(tangent, normal);
	vec3 N = normal;
	mat3 TBN = mtxFromRows(T, B, N);

	lightDirectionTS = mul(TBN, lightDirection);
	viewDirectionTS = mul(TBN, viewDirection);
}

