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

#include "OWApplication.h"

#include <OgreConfigFile.h>
#include <OgreRenderWindow.h>
#include <OgreEntity.h>
#include <OgreTextureManager.h>
#include <OgreResourceGroupManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreMovablePlane.h>
#include <OgreMaterialManager.h>
#include <OgreSubEntity.h>
#include <OgreCompositorManager.h>
#include <OgreParticleSystem.h>
#include <OgreParticle.h>
#include <OgreBillboardParticleRenderer.h>

#include <OIS/OISKeyboard.h>
#include <OIS/OISMouse.h>

namespace OgreWater
{
	Application::Application(void)
		: mRoot(0),
		mPluginsCfg(Ogre::StringUtil::BLANK),
		mResourcesCfg(Ogre::StringUtil::BLANK),
		mWindow(0),
		mWater(0)
	{
	}

	Application::~Application(void)
	{
		delete mWater;

		if (mWindow != 0)
		{
			Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
			windowClosed(mWindow);
		}
		delete mRoot;
	}

	bool Application::go(void)
	{
#ifdef _DEBUG
		mResourcesCfg = "resources_d.cfg";
		mPluginsCfg = "plugins_d.cfg";
#else
		mResourcesCfg = "resources.cfg";
		mPluginsCfg = "plugins.cfg";
#endif
		// construct Ogre::Root
		mRoot = new Ogre::Root(mPluginsCfg);

		// setup resources
		// Load resource paths from config file
		Ogre::ConfigFile cf;
		cf.load(mResourcesCfg);


		// Go through all sections & settings in the file
		Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

		Ogre::String secName, typeName, archName;
		while (seci.hasMoreElements())
		{
			secName = seci.peekNextKey();
			Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
			Ogre::ConfigFile::SettingsMultiMap::iterator i;
			for (i = settings->begin(); i != settings->end(); ++i)
			{
				typeName = i->first;
				archName = i->second;
				Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
					archName, typeName, secName);
			}
		}

		// configure
		// Show the configuration dialog and initialise the system
		if(!mRoot->showConfigDialog())
		{
			return false;
		}

		mWindow = mRoot->initialise(true, "OgreWater Demo v. 1.1");

		// Set default mipmap level (NB some APIs ignore this)
		Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
		// initialise all resource groups
		Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

		// Create the SceneManager, in this case a generic one
		mSceneMgr = mRoot->createSceneManager("DefaultSceneManager");
		mSceneMgr->setSkyDome(true, "Examples/CloudySky", 10.0, 8.0, 1000000000.0);

		mCameraPosition = mSceneMgr->getRootSceneNode()->createChildSceneNode();
		mCameraPitch = mCameraPosition->createChildSceneNode();

		// Create the camera
		mCamera = mSceneMgr->createCamera("Camera");
		mCameraPitch->attachObject(mCamera);

		mCameraPosition->setPosition(Ogre::Vector3(0,500,500));
		mCamera->setNearClipDistance(1);
		mCamera->setFarClipDistance(0);

		// Create one viewport, entire window
		Ogre::Viewport* vp = mWindow->addViewport(mCamera);
		vp->setBackgroundColour(Ogre::ColourValue::Black);

		// Alter the camera aspect ratio to match the viewport
		mCamera->setAspectRatio(
			Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));

		// Create a light
		Ogre::Vector3 lightDir(0.0, -1.0, 0.0);
		lightDir.normalise();

		Ogre::Light* light = mSceneMgr->createLight("MainLight");
		light->setType(Ogre::Light::LT_DIRECTIONAL);
		light->setDirection(lightDir);
		light->setDiffuseColour(Ogre::ColourValue::White);
		light->setSpecularColour(Ogre::ColourValue(0.4, 0.4, 0.4));

		mSceneMgr->setAmbientLight(Ogre::ColourValue(0.2, 0.2, 0.2));

		// Create terrain
		mTerrainGlobals = OGRE_NEW Ogre::TerrainGlobalOptions();

		mTerrainGroup = OGRE_NEW Ogre::TerrainGroup(mSceneMgr, Ogre::Terrain::ALIGN_X_Z, 513, 12000.0f);
		mTerrainGroup->setFilenameConvention(Ogre::String("BasicTutorial3Terrain"), Ogre::String("dat"));
		mTerrainGroup->setOrigin(Ogre::Vector3::ZERO);

		configureTerrainDefaults(light);

		for (long x = 0; x <= 0; ++x)
			for (long y = 0; y <= 0; ++y)
				defineTerrain(x, y);

		// sync load since we want everything in place when we start
		mTerrainGroup->loadAllTerrains(true);

		while (!mTerrainsImported);

		if (mTerrainsImported)
		{
			Ogre::TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
			while(ti.hasMoreElements())
			{
				Ogre::Terrain* t = ti.getNext()->instance;
				initBlendMaps(t);
			}
		}

		mTerrainGroup->update(true);
		mTerrainGroup->freeTemporaryResources();

		// Create water
		mWater = new Water(mWindow, mSceneMgr, mCamera);
		mWater->setWaterDustEnabled(true);
		//mWater->setAirBubblesEnabled(true);
		mWater->init();
		mWater->setWaterHeight(300.0);

		// Initialize OIS
		Ogre::LogManager::getSingletonPtr()->logMessage("*** Initializing OIS ***");
		OIS::ParamList pl;
		size_t windowHnd = 0;
		std::ostringstream windowHndStr;

		mWindow->getCustomAttribute("WINDOW", &windowHnd);
		windowHndStr << windowHnd;
		pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

		mInputManager = OIS::InputManager::createInputSystem( pl );

		mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject( OIS::OISKeyboard, false ));
		mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject( OIS::OISMouse, false ));

		//Set initial mouse clipping size
		windowResized(mWindow);

		//Register as a Window listener
		Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

		Ogre::Timer timer;
		Ogre::Real timeSinceStart = 0.0;

		while(true)
		{
			Ogre::Real timeSinceLastFrame = 0.000001 * timer.getMicroseconds();
			timer.reset();

			timeSinceStart += timeSinceLastFrame;

			// Pump window messages for nice behaviour
			Ogre::WindowEventUtilities::messagePump();

			if(mWindow->isClosed())
			{
				return false;
			}

			mKeyboard->capture();
			mMouse->capture();

			if (mKeyboard->isKeyDown(OIS::KC_ESCAPE))
			{
				return false;
			}

			if (mKeyboard->isKeyDown(OIS::KC_SYSRQ))
			{
				mWater->_writeTexturesToFile();
			}

			// Update water color
			if (!mKeyboard->isModifierDown(OIS::Keyboard::Ctrl))
			{
				Ogre::Vector4 waterFogColor = mWater->getWaterFogColor();

				bool colorUpdated = false;
				if (mKeyboard->isKeyDown(OIS::KC_1) || mKeyboard->isKeyDown(OIS::KC_NUMPAD1))
				{
					waterFogColor.x -= 0.1 * timeSinceLastFrame;
					colorUpdated = true;
				}
				if (mKeyboard->isKeyDown(OIS::KC_2) || mKeyboard->isKeyDown(OIS::KC_NUMPAD2))
				{
					waterFogColor.x += 0.1 * timeSinceLastFrame;
					colorUpdated = true;
				}
				if (mKeyboard->isKeyDown(OIS::KC_3) || mKeyboard->isKeyDown(OIS::KC_NUMPAD3))
				{
					waterFogColor.y -= 0.1 * timeSinceLastFrame;
					colorUpdated = true;
				}
				if (mKeyboard->isKeyDown(OIS::KC_4) || mKeyboard->isKeyDown(OIS::KC_NUMPAD4))
				{
					waterFogColor.y += 0.1 * timeSinceLastFrame;
					colorUpdated = true;
				}
				if (mKeyboard->isKeyDown(OIS::KC_5) || mKeyboard->isKeyDown(OIS::KC_NUMPAD5))
				{
					waterFogColor.z -= 0.1 * timeSinceLastFrame;
					colorUpdated = true;
				}
				if (mKeyboard->isKeyDown(OIS::KC_6) || mKeyboard->isKeyDown(OIS::KC_NUMPAD6))
				{
					waterFogColor.z += 0.1 * timeSinceLastFrame;
					colorUpdated = true;
				}

				if (colorUpdated)
				{
					waterFogColor.x = Ogre::Math::Clamp(waterFogColor.x, 0.0, 1.0);
					waterFogColor.y = Ogre::Math::Clamp(waterFogColor.y, 0.0, 1.0);
					waterFogColor.z = Ogre::Math::Clamp(waterFogColor.z, 0.0, 1.0);

					mWater->setWaterFogColor(waterFogColor);
				}

				if (mKeyboard->isKeyDown(OIS::KC_7) || mKeyboard->isKeyDown(OIS::KC_NUMPAD7))
				{
					mWater->setWaterHeight(mWater->getWaterHeight() - 10.0 * timeSinceLastFrame);
				}
				if (mKeyboard->isKeyDown(OIS::KC_8) || mKeyboard->isKeyDown(OIS::KC_NUMPAD8))
				{
					mWater->setWaterHeight(mWater->getWaterHeight() + 10.0 * timeSinceLastFrame);
				}
			}

			// Update material variables
			if (mKeyboard->isModifierDown(OIS::Keyboard::Ctrl))
			{
				Ogre::Vector4 materialVariables = mWater->getMaterialVariables();
				bool materialUpdated = false;
				if (mKeyboard->isKeyDown(OIS::KC_1) || mKeyboard->isKeyDown(OIS::KC_NUMPAD1))
				{
					materialVariables.x -= 0.1 * timeSinceLastFrame;
					materialUpdated = true;
				}
				if (mKeyboard->isKeyDown(OIS::KC_2) || mKeyboard->isKeyDown(OIS::KC_NUMPAD2))
				{
					materialVariables.x += 0.1 * timeSinceLastFrame;
					materialUpdated = true;
				}
				if (mKeyboard->isKeyDown(OIS::KC_3) || mKeyboard->isKeyDown(OIS::KC_NUMPAD3))
				{
					materialVariables.y -= 0.0001 * timeSinceLastFrame;
					materialUpdated = true;
				}
				if (mKeyboard->isKeyDown(OIS::KC_4) || mKeyboard->isKeyDown(OIS::KC_NUMPAD4))
				{
					materialVariables.y += 0.0001 * timeSinceLastFrame;
					materialUpdated = true;
				}
				if (mKeyboard->isKeyDown(OIS::KC_5) || mKeyboard->isKeyDown(OIS::KC_NUMPAD5))
				{
					materialVariables.z -= 0.001 * timeSinceLastFrame;
					materialUpdated = true;
				}
				if (mKeyboard->isKeyDown(OIS::KC_6) || mKeyboard->isKeyDown(OIS::KC_NUMPAD6))
				{
					materialVariables.z += 0.001 * timeSinceLastFrame;
					materialUpdated = true;
				}
				if (mKeyboard->isKeyDown(OIS::KC_7) || mKeyboard->isKeyDown(OIS::KC_NUMPAD7))
				{
					materialVariables.w -= 0.001 * timeSinceLastFrame;
					materialUpdated = true;
				}
				if (mKeyboard->isKeyDown(OIS::KC_8) || mKeyboard->isKeyDown(OIS::KC_NUMPAD8))
				{
					materialVariables.w += 0.001 * timeSinceLastFrame;
					materialUpdated = true;
				}

				if (materialUpdated)
				{
					materialVariables.x = Ogre::Math::Clamp(materialVariables.x, 0.0, 1.0);
					materialVariables.y = Ogre::Math::Clamp(materialVariables.y, 0.0, 1.0);
					materialVariables.z = Ogre::Math::Clamp(materialVariables.z, 0.0, 1.0);
					materialVariables.w = Ogre::Math::Clamp(materialVariables.w, 0.0, 1.0);

					mWater->setMaterialVariables(materialVariables);
				}
			}

			Ogre::Vector3 cameraPosition = mCameraPosition->getPosition();
			if (mKeyboard->isKeyDown(OIS::KC_A))
			{
				cameraPosition = cameraPosition + 100 * timeSinceLastFrame * -mCamera->getDerivedRight();
			}
			if (mKeyboard->isKeyDown(OIS::KC_D))
			{
				cameraPosition = cameraPosition + 100 * timeSinceLastFrame * mCamera->getDerivedRight();
			}
			if (mKeyboard->isKeyDown(OIS::KC_W))
			{
				cameraPosition = cameraPosition + 100 * timeSinceLastFrame * mCamera->getDerivedDirection();
			}
			if (mKeyboard->isKeyDown(OIS::KC_S))
			{
				cameraPosition = cameraPosition + 100 * timeSinceLastFrame * -mCamera->getDerivedDirection();
			}
			mCameraPosition->setPosition(cameraPosition);

			if (mKeyboard->isKeyDown(OIS::KC_Q))
			{
				mCameraPosition->roll(10 * timeSinceLastFrame * Ogre::Degree(1));
			}
			if (mKeyboard->isKeyDown(OIS::KC_E))
			{
				mCameraPosition->roll(10 * timeSinceLastFrame * Ogre::Degree(-1));
			}

			mCameraPosition->yaw(10 * timeSinceLastFrame * Ogre::Degree(-mMouse->getMouseState().X.rel));
			mCameraPitch->pitch(10 * timeSinceLastFrame * Ogre::Degree(-mMouse->getMouseState().Y.rel));

			Ogre::Vector3 lightDir(Ogre::Math::Sin(0.1 * timeSinceStart), -0.5, 0.0);
			lightDir.normalise();
			light->setDirection(lightDir);

			mWater->update(timeSinceLastFrame);

			// Render a frame
			if(!mRoot->renderOneFrame()) return false;
		}

		// Never reached
		return true;
	}

	//Adjust mouse clipping area
	void Application::windowResized(Ogre::RenderWindow* rw)
	{
		unsigned int width, height, depth;
		int left, top;
		rw->getMetrics(width, height, depth, left, top);

		const OIS::MouseState &ms = mMouse->getMouseState();
		ms.width = width;
		ms.height = height;
	}

	//Unattach OIS before window shutdown (very important under Linux)
	void Application::windowClosed(Ogre::RenderWindow* rw)
	{
		//Only close for window that created OIS (the main window in these demos)
		if( rw == mWindow )
		{
			if( mInputManager )
			{
				mInputManager->destroyInputObject( mMouse );
				mInputManager->destroyInputObject( mKeyboard );

				OIS::InputManager::destroyInputSystem(mInputManager);
				mInputManager = 0;
			}
		}
	}

	void Application::configureTerrainDefaults(Ogre::Light * light)
	{
		// Configure global
		mTerrainGlobals->setMaxPixelError(2.0);
		// testing composite map
		mTerrainGlobals->setCompositeMapDistance(10000);

		// Important to set these so that the terrain knows what to use for derived (non-realtime) data
		mTerrainGlobals->setLightMapDirection(light->getDerivedDirection());
		mTerrainGlobals->setCompositeMapAmbient(mSceneMgr->getAmbientLight());
		mTerrainGlobals->setCompositeMapDiffuse(light->getDiffuseColour());

		// Configure default import settings for if we use imported image
		Ogre::Terrain::ImportData& defaultimp = mTerrainGroup->getDefaultImportSettings();
		defaultimp.terrainSize = 513;
		defaultimp.worldSize = 12000.0f;
		defaultimp.inputScale = 600;
		defaultimp.minBatchSize = 33;
		defaultimp.maxBatchSize = 65;
		// textures
		defaultimp.layerList.resize(3);
		defaultimp.layerList[0].worldSize = 100;
		defaultimp.layerList[0].textureNames.push_back("dirt_grayrocky_diffusespecular.dds");
		defaultimp.layerList[0].textureNames.push_back("dirt_grayrocky_normalheight.dds");
		defaultimp.layerList[1].worldSize = 30;
		defaultimp.layerList[1].textureNames.push_back("grass_green-01_diffusespecular.dds");
		defaultimp.layerList[1].textureNames.push_back("grass_green-01_normalheight.dds");
		defaultimp.layerList[2].worldSize = 200;
		defaultimp.layerList[2].textureNames.push_back("growth_weirdfungus-03_diffusespecular.dds");
		defaultimp.layerList[2].textureNames.push_back("growth_weirdfungus-03_normalheight.dds");
	}

	void Application::defineTerrain(long x, long y)
	{
		Ogre::String filename = mTerrainGroup->generateFilename(x, y);
		if (Ogre::ResourceGroupManager::getSingleton().resourceExists(mTerrainGroup->getResourceGroup(), filename))
		{
			mTerrainGroup->defineTerrain(x, y);
		}
		else
		{
			Ogre::Image img;
			getTerrainImage(x % 2 != 0, y % 2 != 0, img);
			mTerrainGroup->defineTerrain(x, y, &img);
			mTerrainsImported = true;
		}
	}

	void Application::initBlendMaps(Ogre::Terrain* terrain)
	{
		Ogre::TerrainLayerBlendMap* blendMap0 = terrain->getLayerBlendMap(1);
		Ogre::TerrainLayerBlendMap* blendMap1 = terrain->getLayerBlendMap(2);
		Ogre::Real minHeight0 = 70;
		Ogre::Real fadeDist0 = 40;
		Ogre::Real minHeight1 = 70;
		Ogre::Real fadeDist1 = 15;
		float* pBlend1 = blendMap1->getBlendPointer();
		for (Ogre::uint16 y = 0; y < terrain->getLayerBlendMapSize(); ++y)
		{
			for (Ogre::uint16 x = 0; x < terrain->getLayerBlendMapSize(); ++x)
			{
				Ogre::Real tx, ty;

				blendMap0->convertImageToTerrainSpace(x, y, &tx, &ty);
				Ogre::Real height = terrain->getHeightAtTerrainPosition(tx, ty);
				Ogre::Real val = (height - minHeight0) / fadeDist0;
				val = Ogre::Math::Clamp(val, (Ogre::Real)0, (Ogre::Real)1);

				val = (height - minHeight1) / fadeDist1;
				val = Ogre::Math::Clamp(val, (Ogre::Real)0, (Ogre::Real)1);
				*pBlend1++ = val;
			}
		}
		blendMap0->dirty();
		blendMap1->dirty();
		blendMap0->getParent()->updateCompositeMap();
		blendMap1->getParent()->updateCompositeMap();
	}

	void Application::getTerrainImage(bool flipX, bool flipY, Ogre::Image& img)
	{
		img.load("terrain.png", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		if (flipX)
			img.flipAroundY();
		if (flipY)
			img.flipAroundX();
	}
}