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

SAMPLER2D(RT, 0);

OGRE_UNIFORMS(
uniform vec4 viewportSize;
)
const float gaussianWeights[11] = float[](
    0.01222447,
    0.02783468,
    0.06559061,
    0.12097757,
    0.17466632,
    0.19741265,
    0.17466632,
    0.12097757,
    0.06559061,
    0.02783468,
    0.01222447
);
MAIN_PARAMETERS
IN(vec2 vTexCoord, TEXCOORD0)
MAIN_DECLARATION
{
    vec3 blurColor = vec3(0.0);
    
    // Horizontal Gaussian blur
    for (int i = 0; i < 11; ++i) {
        float offset = float(i - 5) * viewportSize.x;
        blurColor += gaussianWeights[i] * texture2D(RT, vec2(vTexCoord.x + offset, vTexCoord.y)).rgb;
    }

    gl_FragColor = vec4(blurColor, 1.0);
}

