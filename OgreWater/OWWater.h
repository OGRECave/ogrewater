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

#ifndef OWWATER_H
#define OWWATER_H

#include "OWAnimatedMesh.h"

#include <OgreRenderTexture.h>
#include <OgreRenderTargetListener.h>
#include <OgreCompositorInstance.h>

namespace OgreWater
{
	class Water
		: public Ogre::RenderTargetListener
		, public Ogre::CompositorInstance::Listener
		, public Ogre::RenderQueue::RenderableListener
	{
	public:
		Water(const Ogre::RenderWindow * window, Ogre::SceneManager * sceneMgr, Ogre::Camera * camera);
		~Water();
		void init();
		void update(Ogre::Real timeSinceLastFrame);

		Ogre::Vector4 getWaterFogColor();
		void setWaterFogColor(Ogre::Vector4 waterFogColor);

		Ogre::Vector4 getMaterialVariables();
		void setMaterialVariables(Ogre::Vector4);

		void setWaterDustEnabled(bool isEnabled);
		bool getWaterDustEnabled() { return mWaterDustEnabled; }
		void setAirBubblesEnabled(bool isEnabled);
		bool getAirBubblesEnabled() { return mAirBubblesEnabled; }

		void _writeTexturesToFile();

	protected:
		// Ogre::RenderTargetListener
		void preRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);
		void postRenderTargetUpdate(const Ogre::RenderTargetEvent &evt);
		// Ogre::CompositorInstance::Listener
		void notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat);
		// Ogre::RenderQueue::RenderableListener
		bool renderableQueued(Ogre::Renderable *rend, Ogre::uint8 groupID, Ogre::ushort priority, Ogre::Technique **ppTech, Ogre::RenderQueue *pQueue);

	private:
		void createWaterDust();
		void destroyWaterDust();

		void createAirBubbles();
		void destroyAirBubbles();

		const Ogre::RenderWindow * mWindow;
		Ogre::SceneManager * mSceneMgr;
		Ogre::Camera * mCamera;

		// Shader variables
		Ogre::Vector4 mWaterFogColor;
		Ogre::Vector4 mMaterialVariables;

		// Reflection
		Ogre::RenderTexture * mReflectionTexture;
		Ogre::Camera * mReflectionCamera;

		// Reflection depth
		Ogre::RenderTexture * mReflectionDepthTexture;
		Ogre::Camera * mReflectionDepthCamera;

		// Refraction
		Ogre::RenderTexture * mRefractionTexture;
		Ogre::Camera * mRefractionCamera;

		// Refraction depth
		Ogre::RenderTexture * mRefractionDepthTexture;
		Ogre::Camera * mRefractionDepthCamera;

		// Scene depth
		Ogre::RenderTexture * mSceneDepthTexture;
		Ogre::Camera * mSceneDepthCamera;

		Ogre::Entity * mWaterPlaneEntity;

		Ogre::Plane mReflectionPlane;
		Ogre::Plane mReflectionClipPlaneAbove;
		Ogre::Plane mReflectionClipPlaneBelow;
		Ogre::Plane mRefractionClipPlaneAbove;
		Ogre::Plane mRefractionClipPlaneBelow;

		Ogre::Technique * mDepthTechnique;
		Ogre::Technique * mWaterDustDepthTechnique;
		Ogre::Pass * mFogPass;
		bool mInDepthPass;

		bool mAboveSurface;
		bool mInRenderTextureUpdate;

		Ogre::CompositorInstance * mWaterFogCompositorInstance;
		Ogre::ParticleSystem * mWaterDustParticleSystem;
		Ogre::SceneNode * mWaterDustSceneNode;
		Ogre::ParticleSystem * mAirBubblesParticleSystem;
		Ogre::SceneNode * mAirBubblesSceneNode;

		AnimatedMesh * mAnimatedMesh;
		Ogre::SceneNode * mWaterPlaneNode;

		bool mWaterDustEnabled;
		bool mAirBubblesEnabled;

		Ogre::Matrix4 aboveSurfaceRefractionMatrix;
		Ogre::Matrix4 belowSurfaceRefractionMatrix;
	};
}

#endif // OWWATER_H
