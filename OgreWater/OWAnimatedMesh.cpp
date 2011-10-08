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

#include "OWAnimatedMesh.h"

#include <OgreMeshManager.h>
#include <OgreHardwareBufferManager.h>
#include <OgreHardwareBuffer.h>
#include <OgreSubMesh.h>

namespace OgreWater
{
	AnimatedMesh::AnimatedMesh(Ogre::String meshName, const Ogre::Real radius, const int rings, const int segments)
		: mMeshName(meshName),
		mRadius(radius),
		mRings(rings),
		mSegments(segments),
		mTotalTime(0),
		mNumVertices(rings * segments + 1),
		mNumIndices(6 * (rings - 1) * segments + 3 * segments) // rings-1 rings of quads, 1 ring of tris
	{
		mData = new Ogre::Vector3[mNumVertices];

		mPerlin.SetOctaveCount(2);

		mMesh = Ogre::MeshManager::getSingleton().createManual(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		Ogre::SubMesh * subMesh = mMesh->createSubMesh();
		Ogre::VertexData * vertexData = mMesh->sharedVertexData = new Ogre::VertexData();

		Ogre::VertexDeclaration * vertexDecl = vertexData->vertexDeclaration;
		size_t currOffset = 0;

		// Positions
		vertexDecl->addElement(0, currOffset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
		currOffset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
		// normals
		vertexDecl->addElement(0, currOffset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
		currOffset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
		// texture coords
		vertexDecl->addElement(0, currOffset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES);
		currOffset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);
		// tangents
		vertexDecl->addElement(0, currOffset, Ogre::VET_FLOAT3, Ogre::VES_TANGENT);
		currOffset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);

		// allocate vertex buffer
		vertexData->vertexCount = mNumVertices;
		mVBuf = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(vertexDecl->getVertexSize(0), vertexData->vertexCount, Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
		Ogre::VertexBufferBinding * binding = vertexData->vertexBufferBinding;
		binding->setBinding(0, mVBuf);
		float* pVertex = static_cast<float*>(mVBuf->lock(Ogre::HardwareBuffer::HBL_DISCARD));

		// allocate index buffer
		subMesh->indexData->indexCount = mNumIndices;
		subMesh->indexData->indexBuffer = Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT, subMesh->indexData->indexCount, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);
		Ogre::HardwareIndexBufferSharedPtr iBuf = subMesh->indexData->indexBuffer;
		unsigned short* pIndices = static_cast<unsigned short*>(iBuf->lock(Ogre::HardwareBuffer::HBL_DISCARD));

		int vertexCount = 0;
		int indexCount = 1;

		Ogre::AxisAlignedBox aabb;
		mMaxRadius = radius;

		for (int ring = 0; ring <= rings; ring++)
		{
			for (int segment = 0; segment < segments; segment++)
			{
				if (ring == 0)
				{
					mData[vertexCount++] = Ogre::Vector3::ZERO;

					*pVertex++ = 0;
					*pVertex++ = 0;
					*pVertex++ = 0;

					Ogre::Vector3 vNormal = Ogre::Vector3::UNIT_Y;
					*pVertex++ = vNormal.x;
					*pVertex++ = vNormal.y;
					*pVertex++ = vNormal.z;

					*pVertex++ = 0.0;
					*pVertex++ = 0.0;

					break;
				}
				else
				{
					Ogre::Real phi = Ogre::Math::TWO_PI * (Ogre::Real(segment) / segments);

					if (segment == 0)
					{
						if (ring == 1)
						{
							mMaxRadius = radius;
						}
						else
						{
							mMaxRadius += (Ogre::Math::TWO_PI * mMaxRadius) / mSegments;
						}
					}

					Ogre::Vector3 pos = mData[vertexCount++] = mMaxRadius * Ogre::Vector3(Ogre::Math::Sin(phi), 0, Ogre::Math::Cos(phi));
					aabb.merge(pos);

					*pVertex++ = pos.x;
					*pVertex++ = pos.y;
					*pVertex++ = pos.z;

					Ogre::Vector3 vNormal = Ogre::Vector3::UNIT_Y;
					*pVertex++ = vNormal.x;
					*pVertex++ = vNormal.y;
					*pVertex++ = vNormal.z;

					*pVertex++ = pos.x;
					*pVertex++ = pos.z;

					*pVertex++ = 0.0;
					*pVertex++ = 0.0;
					*pVertex++ = 0.0;
				}

				if (ring > 0)
				{
					if (ring == 1)
					{
						if (segment < (segments-1))
						{
							*pIndices++ = 0;
							*pIndices++ = indexCount;
							*pIndices++ = indexCount + 1;
						}
						else
						{
							*pIndices++ = 0;
							*pIndices++ = indexCount;
							*pIndices++ = 1;
						}
					}
					else
					{
						if (segment < (segments-1))
						{
							*pIndices++ = indexCount - segments;
							*pIndices++ = indexCount;               
							*pIndices++ = indexCount + 1;
							*pIndices++ = indexCount - segments;
							*pIndices++ = indexCount + 1;
							*pIndices++ = indexCount - segments + 1;
						}
						else
						{
							*pIndices++ = indexCount - segments;
							*pIndices++ = indexCount;               
							*pIndices++ = indexCount - segments + 1;
							*pIndices++ = indexCount - segments;
							*pIndices++ = indexCount - segments + 1;
							*pIndices++ = indexCount - 2 * segments + 1;
						}
					}
					indexCount++;
				}
			}
		}

		// Unlock
		mVBuf->unlock();
		iBuf->unlock();
		// Generate face list
		subMesh->useSharedVertices = true;

		// the original code was missing this line:
		mMesh->_setBounds(aabb, false );
		mMesh->_setBoundingSphereRadius(aabb.getMaximum().length());
		// this line makes clear the mesh is loaded (avoids memory leaks)
		mMesh->load();
	}

	AnimatedMesh::~AnimatedMesh()
	{
	}

	void AnimatedMesh::update(Ogre::Real timeSinceLastFrame, Ogre::Vector3 basePos)
	{
		mTotalTime += timeSinceLastFrame;

		Ogre::AxisAlignedBox aabb;

		for (int i = 0; i < mNumVertices; i++)
		{
			Ogre::Real r = Ogre::Vector3(mData[i].x, 0.0, mData[i].z).length();
			Ogre::Real k = 1.0 - r / mMaxRadius;
			Ogre::Vector3 pos = basePos + mData[i];
			Ogre::Real height = k * 25 * mPerlin.GetValue(0.005 * pos.x + 0.5 * mTotalTime, 0.2 * mTotalTime, 0.001 * pos.z);

			mData[i].y = height;

			aabb.merge(mData[i]);
		}

		int vertexCount = 0;

		float* pVertex = static_cast<float*>(mVBuf->lock(Ogre::HardwareBuffer::HBL_DISCARD));
		for (int ring = 0; ring <= mRings; ring++)
		{
			for (int segment = 0; segment < mSegments; segment++)
			{
				if (ring == 0)
				{
					Ogre::Vector3 pos = mData[vertexCount++];

					*pVertex++ = pos.x;
					*pVertex++ = pos.y;
					*pVertex++ = pos.z;

					Ogre::Vector3 vNormal = Ogre::Vector3::ZERO;

					for (int i = 0; i < mSegments; i++)
					{
						Ogre::Vector3 vec1 = mData[(i % mSegments) + 1] - mData[0];
						Ogre::Vector3 vec2 = mData[((i + 1) % mSegments) + 1] - mData[0];
						vNormal += vec1.crossProduct(vec2).normalisedCopy();
					}

					vNormal.normalise();
					*pVertex++ = vNormal.x;
					*pVertex++ = vNormal.y;
					*pVertex++ = vNormal.z;

					*pVertex++ = pos.x;
					*pVertex++ = pos.z;

					*pVertex++ = 0.0;
					*pVertex++ = 0.0;
					*pVertex++ = 0.0;

					break;
				}
				else
				{
					Ogre::Vector3 pos = mData[vertexCount];

					*pVertex++ = pos.x;
					*pVertex++ = pos.y;
					*pVertex++ = pos.z;

					Ogre::Vector3 vNormal = Ogre::Vector3::ZERO;

					if (ring == 1)
					{
						Ogre::Vector3 vec1 = mData[(segment + 1) % mSegments + 1] - mData[segment + 1];
						Ogre::Vector3 vec2 = mData[0] - mData[segment + 1];
						vNormal += vec1.crossProduct(vec2).normalisedCopy();

						vec1 = mData[0] - mData[segment + 1];
						vec2 = mData[(segment + mSegments - 1) % mSegments + 1] - mData[segment + 1];
						vNormal += vec1.crossProduct(vec2).normalisedCopy();

						if (mRings > 1)
						{
							vec1 = mData[mSegments + segment + 1] - mData[segment + 1];
							vec2 = mData[(segment + 1) % mSegments + 1] - mData[segment + 1];
							vNormal += vec1.crossProduct(vec2).normalisedCopy();

							vec1 = mData[(segment + mSegments - 1) % mSegments + 1] - mData[segment + 1];
							vec2 = mData[mSegments + segment + 1] - mData[segment + 1];
							vNormal += vec1.crossProduct(vec2).normalisedCopy();
						}
					}
					else if (ring < mRings)
					{
						Ogre::Vector3 vec1 =
							mData[ring * mSegments + (segment + 1) % mSegments + 1] -
							mData[ring * mSegments + segment + 1];
						Ogre::Vector3 vec2 =
							mData[(ring - 1) * mSegments + segment + 1] -
							mData[ring * mSegments + segment + 1];
						vNormal += vec1.crossProduct(vec2).normalisedCopy();

						vec1 =
							mData[(ring - 1) * mSegments + segment + 1] -
							mData[ring * mSegments + segment + 1];
						vec2 =
							mData[ring * mSegments + (segment + mSegments - 1) % mSegments + 1] -
							mData[ring * mSegments + segment + 1];
						vNormal += vec1.crossProduct(vec2).normalisedCopy();

						if (ring < (mRings - 1))
						{
							vec1 =
								mData[(ring + 1) * mSegments + segment + 1] -
								mData[ring * mSegments + segment + 1];
							vec2 =
								mData[ring * mSegments + (segment + 1) % mSegments + 1] -
								mData[ring * mSegments + segment + 1];
							vNormal += vec1.crossProduct(vec2).normalisedCopy();

							vec1 =
								mData[ring * mSegments + (segment + mSegments - 1) % mSegments + 1] -
								mData[ring * mSegments + segment + 1];
							vec2 =
								mData[(ring + 1) * mSegments + segment + 1] -
								mData[ring * mSegments + segment + 1];
							vNormal += vec1.crossProduct(vec2).normalisedCopy();
						}
					}

					vNormal.normalise();

					*pVertex++ = vNormal.x;
					*pVertex++ = vNormal.y;
					*pVertex++ = vNormal.z;

					*pVertex++ = pos.x;
					*pVertex++ = pos.z;

					*pVertex++ = 0.0;
					*pVertex++ = 0.0;
					*pVertex++ = 0.0;

					vertexCount++;
				}
			}
		}

		mVBuf->unlock();

		mMesh->_setBounds(aabb, false);
		mMesh->_setBoundingSphereRadius(aabb.getMaximum().length());

		unsigned short src, dest;
		if (!mMesh->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
		{
			mMesh->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
		} else {
			mMesh->buildTangentVectors();
		}
	}
}