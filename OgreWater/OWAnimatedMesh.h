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

#ifndef OWANIMATEDMESH_H
#define OWANIMATEDMESH_H

#include <OgreMesh.h>

#include <noise.h>

namespace OgreWater
{
	class AnimatedMesh
	{
	public:
		AnimatedMesh(Ogre::String meshName, const Ogre::Real radius, const int rings, const int segments);
		virtual ~AnimatedMesh();
		void update(Ogre::Real timeSinceLastFrame, Ogre::Vector3 basePos);

	private:
		Ogre::String mMeshName;
		const Ogre::Real mRadius;
		const int mRings;
		const int mSegments;
		Ogre::Real mTotalTime;
		Ogre::MeshPtr mMesh;
		Ogre::HardwareVertexBufferSharedPtr mVBuf;
		const int mNumVertices;
		const int mNumIndices;
		Ogre::Vector3 * mData;
		Ogre::Real mMaxRadius;

		noise::module::Perlin mPerlin;
		//noise::module::RidgedMulti mPerlin;
	};
}

#endif // OWANIMATEDMESH_H