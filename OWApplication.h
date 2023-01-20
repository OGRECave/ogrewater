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

#ifndef OWAPPLICATION_H
#define OWAPPLICATION_H

#include "OWWater.h"

#include <OgreRoot.h>
#include <OgreWindowEventUtilities.h>
#include <OgreRenderTargetListener.h>
#include <OgreTerrain.h>
#include <OgreTerrainGroup.h>
#include <OgreCompositorInstance.h>

#include <OgreApplicationContext.h>
#include <OgreCameraMan.h>

namespace OgreWater
{
	class Application : public OgreBites::ApplicationContext, public OgreBites::InputListener, public Ogre::RenderTargetListener
	{
		// Camera
		Ogre::SceneNode * mCameraPosition;

		std::unique_ptr<Water> mWater;

		Ogre::SceneManager* mSceneMgr;
		Ogre::Camera * mCamera;

		// Terrain
		Ogre::TerrainGlobalOptions * mTerrainGlobals;
		Ogre::TerrainGroup* mTerrainGroup;
		bool mTerrainsImported;

		OgreBites::CameraMan* mCameraMan;
		Ogre::Real mTimeSinceLastFrame;

    	void preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt) override;

	protected:
		void configureTerrainDefaults(Ogre::Light * light);
		void defineTerrain(long x, long y);
		void initBlendMaps(Ogre::Terrain* terrain);
		void getTerrainImage(bool flipX, bool flipY, Ogre::Image& img);
		void loadResources() override {}
		bool keyPressed(const OgreBites::KeyboardEvent& evt) override;
	public:
		Application(void);
		bool go(void);
	};
}

#endif // OWAPPLICATION_H