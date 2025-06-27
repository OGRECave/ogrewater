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

#include <Ogre.h>
#include <OgreApplicationContext.h>
#include <OgreRTShaderSystem.h>

#include <OgreOverlayManager.h>
#include <OgreOverlaySystem.h>
#include <OgreImGuiOverlay.h>
#include <OgreImGuiInputListener.h>

namespace OgreWater
{
	Application::Application(void)
		: OgreBites::ApplicationContext("OgreWater")
	{
	}

	void Application::preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt)
	{
		Ogre::ImGuiOverlay::NewFrame();

		ImGui::Begin("Water Properties");
		float waterHeight = mWater->getWaterHeight();
		if(ImGui::SliderFloat("Water height", &waterHeight, 100, 1000))
			mWater->setWaterHeight(waterHeight);

		Ogre::Vector4 materialVariables = mWater->getMaterialVariables();
		bool materialUpdated = false;
		if(ImGui::SliderFloat("reflection texture offset", &materialVariables.x, 0, 1))
			materialUpdated = true;
		if(ImGui::SliderFloat("refraction texture offset", &materialVariables.y, 0, 0.1))
			materialUpdated = true;
		if(ImGui::SliderFloat("water density", &materialVariables.z, 0, 0.01, "%.4f"))
			materialUpdated = true;
		if(ImGui::SliderFloat("water blur", &materialVariables.w, 0, 0.01, "%.4f"))
			materialUpdated = true;
		if(materialUpdated)
			mWater->setMaterialVariables(materialVariables);

		Ogre::Vector4 waterFogColor = mWater->getWaterFogColor();
		if(ImGui::ColorEdit3("water fog color", waterFogColor.ptr()))
			mWater->setWaterFogColor(waterFogColor);

		ImGui::End();
	}

	bool Application::keyPressed(const OgreBites::KeyboardEvent& evt)
	{
		if(evt.keysym.sym == OgreBites::SDLK_ESCAPE)
		{
			getRoot()->queueEndRendering();
			return true;
		}

		if (evt.keysym.sym == OgreBites::SDLK_PRINTSCREEN)
		{
			mWater->_writeTexturesToFile();
			return true;
		}

		Ogre::Vector3 cameraPosition = mCameraPosition->getPosition();

		if (evt.keysym.sym == 'q')
		{
			mCameraPosition->roll(10 * mTimeSinceLastFrame * Ogre::Degree(1));
		}
		if (evt.keysym.sym == 'e')
		{
			mCameraPosition->roll(10 * mTimeSinceLastFrame * Ogre::Degree(-1));
		}

		return true;
	}

	bool Application::go(void)
	{
		initApp();

		addInputListener(this);
		auto window = getRenderWindow();

		// Create the SceneManager, in this case a generic one
		mSceneMgr = mRoot->createSceneManager();
		Ogre::RTShader::ShaderGenerator::getSingleton().addSceneManager(mSceneMgr);
		mCameraPosition = mSceneMgr->getRootSceneNode()->createChildSceneNode();
		mSceneMgr->addRenderQueueListener(getOverlaySystem());
		window->addListener(this);

		auto imguiOverlay = new Ogre::ImGuiOverlay();
		imguiOverlay->show();
		Ogre::OverlayManager::getSingleton().addOverlay(imguiOverlay); // now owned by overlaymgr

		// Create the camera
		mCamera = mSceneMgr->createCamera("Camera");
		mCameraPosition->attachObject(mCamera);

		mCamera->setNearClipDistance(1);
		mCamera->setFarClipDistance(0);

		mCameraMan = new OgreBites::CameraMan(mCameraPosition);
		mCameraMan->setStyle(OgreBites::CS_ORBIT);
		mCameraMan->setYawPitchDist(Ogre::Degree(0), Ogre::Degree(20), 1000);
		auto imguiListener = new OgreBites::ImGuiInputListener();
		OgreBites::InputListenerChain chain({imguiListener, mCameraMan});
		addInputListener(&chain);

		// Create one viewport, entire window
		Ogre::Viewport* vp = window->addViewport(mCamera);
		vp->setBackgroundColour(Ogre::ColourValue::Black);

		// Alter the camera aspect ratio to match the viewport
		mCamera->setAspectRatio(
			Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));

		// Create a light
		Ogre::Vector3 lightDir(0.0, -1.0, 0.0);
		lightDir.normalise();

		Ogre::Light* light = mSceneMgr->createLight("MainLight");
		light->setType(Ogre::Light::LT_DIRECTIONAL);

		light->setDiffuseColour(Ogre::ColourValue::White);
		light->setSpecularColour(Ogre::ColourValue(0.4, 0.4, 0.4));

		auto lightNode = mSceneMgr->createSceneNode();
		lightNode->attachObject(light);
		lightNode->setDirection(lightDir);

		mSceneMgr->setAmbientLight(Ogre::ColourValue(0.2, 0.2, 0.2));

		// Create water
		mWater.reset(new Water(window, mSceneMgr, mCamera));
		mWater->createTextures();
		// initialise all resource groups
		Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

		mWater->setWaterDustEnabled(true);
		//mWater->setAirBubblesEnabled(true);
		mWater->init();
		mWater->setWaterHeight(300.0);
		mSceneMgr->setSkyDome(true, "Examples/CloudySky", 10.0, 8.0, 1000000000.0);

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
			auto ti = mTerrainGroup->getTerrainSlots().begin();
			while(ti != mTerrainGroup->getTerrainSlots().end())
			{
				Ogre::Terrain* t = (*ti).second->instance;
				initBlendMaps(t);
				ti++;
			}
		}

		mTerrainGroup->update(true);
		mTerrainGroup->freeTemporaryResources();

		Ogre::Timer timer;
		Ogre::Real timeSinceStart = 0.0;

		while(true)
		{
			mTimeSinceLastFrame = 0.000001 * timer.getMicroseconds();
			timer.reset();

			timeSinceStart += mTimeSinceLastFrame;

			if(mRoot->endRenderingQueued())
			{
				return false;
			}

			Ogre::Vector3 lightDir(Ogre::Math::Sin(0.1 * timeSinceStart), -0.5, 0.0);
			lightDir.normalise();
			lightNode->setDirection(lightDir);

			mWater->update(mTimeSinceLastFrame);

			// Render a frame
			if(!mRoot->renderOneFrame()) return false;
		}

		closeApp();

		return true;
	}

	void Application::configureTerrainDefaults(Ogre::Light * light)
	{
		// Configure global
		mTerrainGlobals->setMaxPixelError(2.0);
		// testing composite map
		mTerrainGlobals->setCompositeMapDistance(10000);
		mTerrainGlobals->setUseVertexCompressionWhenAvailable(false);

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
        Ogre::Image combined;
        combined.loadTwoImagesAsRGBA("Ground23_col.jpg", "Ground23_spec.png", "General");
        Ogre::TextureManager::getSingleton().loadImage("Ground23_diffspec", "General", combined);

        defaultimp.layerList.resize(3);
        defaultimp.layerList[0].worldSize = 200;
        defaultimp.layerList[0].textureNames.push_back("Ground37_diffspec.dds");
        defaultimp.layerList[0].textureNames.push_back("Ground37_normheight.dds");
        defaultimp.layerList[1].worldSize = 200;
        defaultimp.layerList[1].textureNames.push_back("Ground23_diffspec"); // loaded from memory
        defaultimp.layerList[1].textureNames.push_back("Ground23_normheight.dds");
        defaultimp.layerList[2].worldSize = 400;
        defaultimp.layerList[2].textureNames.push_back("Rock20_diffspec.dds");
        defaultimp.layerList[2].textureNames.push_back("Rock20_normheight.dds");
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
		blendMap0->update();
		blendMap1->update();
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
