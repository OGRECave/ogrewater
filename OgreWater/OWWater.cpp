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

#include "OWWater.h"

#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>
#include <OgreMaterialManager.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreCompositorManager.h>
#include <OgreBillboardParticleRenderer.h>
#include <OgreParticleSystem.h>
#include <OgreParticle.h>

namespace OgreWater
{
	Water::Water(const Ogre::RenderWindow * window, Ogre::SceneManager * sceneMgr, Ogre::Camera * camera)
		: mWindow(window),
		mSceneMgr(sceneMgr),
		mCamera(camera),
		mReflectionPlane(Ogre::Vector3(0.0, 1.0, 0.0), 200.0),
		mReflectionClipPlaneAbove(Ogre::Vector3(0.0, 1.0, 0.0), 198.0),
		mReflectionClipPlaneBelow(Ogre::Vector3(0.0, -1.0, 0.0), -202.0),
		mRefractionClipPlaneAbove(Ogre::Vector3(0.0, -1.0, 0.0), -202.0),
		mRefractionClipPlaneBelow(Ogre::Vector3(0.0, 1.0, 0.0), 198.0),
		mInDepthPass(true),
		mAboveSurface(true),
		mWaterDustEnabled(false),
		mAirBubblesEnabled(false)

	{
		mWaterFogColor = Ogre::Vector4(0.0, 0.0, 0.0, 1.0);
		mMaterialVariables = Ogre::Vector4(
			0.0,  // reflection texture offset
			0.0,  // refraction texture offset
			0.0,  // water density
			0.0); // water blur
	}

	Water::~Water()
	{
		mReflectionTexture->removeAllListeners();
		mReflectionDepthTexture->removeAllListeners();
		mRefractionTexture->removeAllListeners();
		mRefractionDepthTexture->removeAllListeners();
		mSceneDepthTexture->removeAllListeners();
	}

	void Water::init()
	{
		// Reflection
		Ogre::TexturePtr reflectionTexture = Ogre::TextureManager::getSingleton().createManual(
			"ReflectionTexture",
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Ogre::TEX_TYPE_2D,
			mWindow->getWidth(),
			mWindow->getHeight(),
			0,
			Ogre::PF_R8G8B8A8,
			Ogre::TU_RENDERTARGET);

		mReflectionTexture = reflectionTexture->getBuffer()->getRenderTarget();
		mReflectionTexture->addListener(this);

		mReflectionCamera = mSceneMgr->createCamera("ReflectionCamera");
		mReflectionCamera->setPosition(mCamera->getDerivedPosition());
		mReflectionCamera->setOrientation(mCamera->getDerivedOrientation());
		mReflectionCamera->setAspectRatio(mCamera->getAspectRatio());
		mReflectionCamera->setNearClipDistance(mCamera->getNearClipDistance());
		mReflectionCamera->setFarClipDistance(mCamera->getFarClipDistance());
		mReflectionCamera->enableCustomNearClipPlane(mReflectionClipPlaneAbove);
		mReflectionCamera->enableReflection(mReflectionPlane);

		Ogre::Viewport * reflectionViewport = mReflectionTexture->addViewport(mReflectionCamera);
		reflectionViewport->setClearEveryFrame(true);
		reflectionViewport->setBackgroundColour(Ogre::ColourValue(0.0, 0.0, 0.0, 0.0));
		reflectionViewport->setOverlaysEnabled(false);

		// Reflection depth
		Ogre::TexturePtr reflectionDepthTexture = Ogre::TextureManager::getSingleton().createManual(
			"ReflectionDepthTexture",
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Ogre::TEX_TYPE_2D,
			mWindow->getWidth(),
			mWindow->getHeight(),
			0,
			Ogre::PF_FLOAT32_R,
			Ogre::TU_RENDERTARGET);

		mReflectionDepthTexture = reflectionDepthTexture->getBuffer()->getRenderTarget();
		mReflectionDepthTexture->addListener(this);

		mReflectionDepthCamera = mSceneMgr->createCamera("ReflectionDepthCamera");
		mReflectionDepthCamera->setPosition(mCamera->getDerivedPosition());
		mReflectionDepthCamera->setOrientation(mCamera->getDerivedOrientation());
		mReflectionDepthCamera->setAspectRatio(mCamera->getAspectRatio());
		mReflectionDepthCamera->setNearClipDistance(mCamera->getNearClipDistance());
		mReflectionDepthCamera->setFarClipDistance(mCamera->getFarClipDistance());
		mReflectionDepthCamera->enableCustomNearClipPlane(mReflectionClipPlaneAbove);
		mReflectionDepthCamera->enableReflection(mReflectionPlane);

		Ogre::Viewport * reflectionDepthViewport = mReflectionDepthTexture->addViewport(mReflectionDepthCamera);
		reflectionDepthViewport->setClearEveryFrame(true);
		reflectionDepthViewport->setBackgroundColour(Ogre::ColourValue(0.0, 0.0, 0.0, 0.0));
		reflectionDepthViewport->setOverlaysEnabled(false);
		reflectionDepthViewport->setSkiesEnabled(false);

		// Refraction
		Ogre::TexturePtr refractionTexture = Ogre::TextureManager::getSingleton().createManual(
			"RefractionTexture",
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Ogre::TEX_TYPE_2D,
			mWindow->getWidth(),
			mWindow->getHeight(),
			0,
			Ogre::PF_R8G8B8A8,
			Ogre::TU_RENDERTARGET);

		mRefractionTexture = refractionTexture->getBuffer()->getRenderTarget();
		mRefractionTexture->addListener(this);

		mRefractionCamera = mSceneMgr->createCamera("RefractionCamera");
		mRefractionCamera->setPosition(mCamera->getDerivedPosition());
		mRefractionCamera->setOrientation(mCamera->getDerivedOrientation());
		mRefractionCamera->setAspectRatio(mCamera->getAspectRatio());
		mRefractionCamera->setNearClipDistance(mCamera->getNearClipDistance());
		mRefractionCamera->setFarClipDistance(mCamera->getFarClipDistance());
		mRefractionCamera->enableCustomNearClipPlane(mRefractionClipPlaneAbove);

		Ogre::Viewport * refractionViewport = mRefractionTexture->addViewport(mRefractionCamera);
		refractionViewport->setClearEveryFrame(true);
		refractionViewport->setBackgroundColour(Ogre::ColourValue(0.0, 0.0, 0.0, 0.0));
		refractionViewport->setOverlaysEnabled(false);
		refractionViewport->setSkiesEnabled(false);

		// Refraction depth
		Ogre::TexturePtr refractionDepthTexture = Ogre::TextureManager::getSingleton().createManual(
			"RefractionDepthTexture",
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Ogre::TEX_TYPE_2D,
			mWindow->getWidth(),
			mWindow->getHeight(),
			0,
			Ogre::PF_FLOAT32_R,
			Ogre::TU_RENDERTARGET);

		mRefractionDepthTexture = refractionDepthTexture->getBuffer()->getRenderTarget();
		mRefractionDepthTexture->addListener(this);

		mRefractionDepthCamera = mSceneMgr->createCamera("RefractionDepthCamera");
		mRefractionDepthCamera->setPosition(mCamera->getDerivedPosition());
		mRefractionDepthCamera->setOrientation(mCamera->getDerivedOrientation());
		mRefractionDepthCamera->setAspectRatio(mCamera->getAspectRatio());
		mRefractionDepthCamera->setNearClipDistance(mCamera->getNearClipDistance());
		mRefractionDepthCamera->setFarClipDistance(mCamera->getFarClipDistance());
		mRefractionDepthCamera->enableCustomNearClipPlane(mRefractionClipPlaneAbove);

		Ogre::Viewport * refractionDepthViewport = mRefractionDepthTexture->addViewport(mRefractionDepthCamera);
		refractionDepthViewport->setClearEveryFrame(true);
		refractionDepthViewport->setBackgroundColour(Ogre::ColourValue(0.0, 0.0, 0.0, 0.0));
		refractionDepthViewport->setOverlaysEnabled(false);
		refractionDepthViewport->setSkiesEnabled(false);

		// Scene depth (later used for full-screen under-water fog)
		Ogre::TexturePtr sceneDepthTexture = Ogre::TextureManager::getSingleton().createManual(
			"SceneDepthTexture",
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Ogre::TEX_TYPE_2D,
			mWindow->getWidth(),
			mWindow->getHeight(),
			0,
			Ogre::PF_FLOAT32_R,
			Ogre::TU_RENDERTARGET);

		mSceneDepthTexture = sceneDepthTexture->getBuffer()->getRenderTarget();
		mSceneDepthTexture->addListener(this);

		mSceneDepthCamera = mSceneMgr->createCamera("SceneDepthCamera");
		mSceneDepthCamera->setPosition(mCamera->getDerivedPosition());
		mSceneDepthCamera->setOrientation(mCamera->getDerivedOrientation());
		mSceneDepthCamera->setAspectRatio(mCamera->getAspectRatio());
		mSceneDepthCamera->setNearClipDistance(mCamera->getNearClipDistance());
		mSceneDepthCamera->setFarClipDistance(mCamera->getFarClipDistance());

		Ogre::Viewport * sceneDepthViewport = mSceneDepthTexture->addViewport(mSceneDepthCamera);
		sceneDepthViewport->setClearEveryFrame(true);
		sceneDepthViewport->setBackgroundColour(Ogre::ColourValue(0.0, 0.0, 0.0, 0.0));
		sceneDepthViewport->setOverlaysEnabled(false);
		sceneDepthViewport->setSkiesEnabled(false);

		// Manually load depth pass
		Ogre::MaterialPtr depthMaterial = Ogre::MaterialManager::getSingleton().getByName("OgreWater/Depth");
		depthMaterial->load();
		mDepthTechnique = depthMaterial->getBestTechnique();

		// Manually load water dust depth pass
		Ogre::MaterialPtr waterDustDepthMaterial = Ogre::MaterialManager::getSingleton().getByName("OgreWater/WaterDustDepth");
		waterDustDepthMaterial->load();
		mWaterDustDepthTechnique = waterDustDepthMaterial->getBestTechnique();

		// Manually load fog pass
		Ogre::MaterialPtr fogMaterial = Ogre::MaterialManager::getSingleton().getByName("OgreWater/Fog");
		fogMaterial->load();
		Ogre::Technique * fogTechnique = fogMaterial->getBestTechnique();
		mFogPass = fogTechnique->getPass(0);

		//mAnimatedMesh = new AnimatedMesh("WaterMesh", 10, 100, 100);
		mWaterPlaneEntity = mSceneMgr->createEntity(Ogre::SceneManager::PT_PLANE);

		mWaterPlaneEntity->setMaterialName("OgreWater/Water/Above");
		for (unsigned int i = 0; i < mWaterPlaneEntity->getNumSubEntities(); i++)
		{
			mWaterPlaneEntity->getSubEntity(i)->setCustomParameter(0, mWaterFogColor);
			mWaterPlaneEntity->getSubEntity(i)->setCustomParameter(1, mMaterialVariables);
		}

		mWaterPlaneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
		mWaterPlaneNode->setPosition(0.0, 200.0, 0.0);
		mWaterPlaneNode->setScale(100 * Ogre::Vector3::UNIT_SCALE);
		mWaterPlaneNode->pitch(Ogre::Degree(-90));
		mWaterPlaneNode->attachObject(mWaterPlaneEntity);

		// Set up Compositor
		mWaterFogCompositorInstance = Ogre::CompositorManager::getSingleton().addCompositor(mCamera->getViewport(), "OgreWater/PostProcessingFog");
		mWaterFogCompositorInstance->addListener(this);

		mSceneMgr->getRenderQueue()->setRenderableListener(this);
	}

	void Water::update(Ogre::Real timeSinceLastFrame)
	{
		//Ogre::Vector3 pos = mCamera->getDerivedPosition();
		//pos.y = 200.0;
		//mWaterPlaneNode->_setDerivedPosition(pos);
		//mAnimatedMesh->update(timeSinceLastFrame, pos);

		mReflectionCamera->setPosition(mCamera->getDerivedPosition());
		mReflectionCamera->setOrientation(mCamera->getDerivedOrientation());
		mReflectionDepthCamera->setPosition(mCamera->getDerivedPosition());
		mReflectionDepthCamera->setOrientation(mCamera->getDerivedOrientation());
		mRefractionCamera->setPosition(mCamera->getDerivedPosition());
		mRefractionCamera->setOrientation(mCamera->getDerivedOrientation());
		mRefractionDepthCamera->setPosition(mCamera->getDerivedPosition());
		mRefractionDepthCamera->setOrientation(mCamera->getDerivedOrientation());
		mSceneDepthCamera->setPosition(mCamera->getDerivedPosition());
		mSceneDepthCamera->setOrientation(mCamera->getDerivedOrientation());

		if (mWaterDustEnabled)
		{
			mWaterDustSceneNode->setPosition(mCamera->getDerivedPosition());

			Ogre::ParticleIterator particleIterator = mWaterDustParticleSystem->_getIterator();

			while (!particleIterator.end())
			{
				Ogre::Particle * particle = particleIterator.getNext();
				Ogre::Vector3 position = particle->position - mWaterDustSceneNode->getPosition();
				if (position.x > 250)
				{
					particle->position.x -= 500;
				}
				else if (position.x < -250)
				{
					particle->position.x += 500;
				}
				if (position.y > 250)
				{
					particle->position.y -= 500;
				}
				else if (position.y < -250)
				{
					particle->position.y += 500;
				}
				if (position.z > 250)
				{
					particle->position.z -= 500;
				}
				else if (position.z < -250)
				{
					particle->position.z += 500;
				}

				while (particle->position.y > 200)
				{
					particle->position.y -= 500;
				}
			}
		}

		if (mAirBubblesEnabled)
		{
			mAirBubblesSceneNode->setPosition(mCamera->getDerivedPosition());

			Ogre::ParticleIterator particleIterator = mAirBubblesParticleSystem->_getIterator();

			while (!particleIterator.end())
			{
				Ogre::Particle * particle = particleIterator.getNext();

				if (particle->position.y > 200)
				{
					particle->timeToLive = 0;
				}
			}
		}

		if (mAboveSurface)
		{
			if (mCamera->getDerivedPosition().y < 200)
			{
				// Switching from above surface to below surface
				Ogre::CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "OgreWater/PostProcessingFog", true);

				mAboveSurface = false;
				mWaterPlaneEntity->setMaterialName("OgreWater/Water/Below");

				// Update custom near clip planes
				mReflectionCamera->enableCustomNearClipPlane(mReflectionClipPlaneBelow);
				mReflectionDepthCamera->enableCustomNearClipPlane(mReflectionClipPlaneBelow);
				mRefractionCamera->enableCustomNearClipPlane(mRefractionClipPlaneBelow);
				mRefractionDepthCamera->enableCustomNearClipPlane(mRefractionClipPlaneBelow);

				mReflectionCamera->getViewport()->setSkiesEnabled(false);
				mRefractionCamera->getViewport()->setSkiesEnabled(true);

				for (unsigned int i = 0; i < mWaterPlaneEntity->getNumSubEntities(); i++)
				{
					mWaterPlaneEntity->getSubEntity(i)->setCustomParameter(0, mWaterFogColor);
					mWaterPlaneEntity->getSubEntity(i)->setCustomParameter(1, mMaterialVariables);
				}
			}
			else
			{
				//// Update refraction camera view matrix 
				//Ogre::Matrix4 refractionViewMatrix = mCamera->getViewMatrix();

				//Ogre::Matrix4 preScalingTranslateMatrix = Ogre::Matrix4(
				//	1, 0, 0, 0,
				//	0, 1.0, 0, -200,
				//	0, 0, 1, 0,
				//	0, 0, 0, 1);

				//Ogre::Matrix4 scalingMatrix = Ogre::Matrix4(
				//	1, 0, 0, 0,
				//	0, 1.0/1.33, 0, 0,
				//	0, 0, 1, 0,
				//	0, 0, 0, 1);

				//Ogre::Matrix4 postScalingTranslateMatrix = Ogre::Matrix4(
				//	1, 0, 0, 0,
				//	0, 1.0, 0, 200,
				//	0, 0, 1, 0,
				//	0, 0, 0, 1);

				//mRefractionCamera->setCustomViewMatrix(true, refractionViewMatrix * postScalingTranslateMatrix * scalingMatrix * preScalingTranslateMatrix);
			}
		}
		else
		{
			if (mCamera->getDerivedPosition().y > 200)
			{
				// Switching from below surface to above surface
				Ogre::CompositorManager::getSingleton().setCompositorEnabled(mCamera->getViewport(), "OgreWater/PostProcessingFog", false);

				mAboveSurface = true;
				mWaterPlaneEntity->setMaterialName("OgreWater/Water/Above");

				// Update custom near clip planes
				mReflectionCamera->enableCustomNearClipPlane(mReflectionClipPlaneAbove);
				mReflectionDepthCamera->enableCustomNearClipPlane(mReflectionClipPlaneAbove);
				mRefractionCamera->enableCustomNearClipPlane(mRefractionClipPlaneAbove);
				mRefractionDepthCamera->enableCustomNearClipPlane(mRefractionClipPlaneAbove);

				mReflectionCamera->getViewport()->setSkiesEnabled(true);
				mRefractionCamera->getViewport()->setSkiesEnabled(false);

				for (unsigned int i = 0; i < mWaterPlaneEntity->getNumSubEntities(); i++)
				{
					mWaterPlaneEntity->getSubEntity(i)->setCustomParameter(0, mWaterFogColor);
					mWaterPlaneEntity->getSubEntity(i)->setCustomParameter(1, mMaterialVariables);
				}

				mRefractionDepthCamera->setCustomViewMatrix(false);
			}
			else
			{
				//// Update refraction camera view matrix 
				//Ogre::Matrix4 refractionViewMatrix = mCamera->getViewMatrix();

				//Ogre::Matrix4 preScalingTranslateMatrix = Ogre::Matrix4(
				//	1, 0, 0, 0,
				//	0, 1.0, 0, -200,
				//	0, 0, 1, 0,
				//	0, 0, 0, 1);

				//Ogre::Matrix4 scalingMatrix = Ogre::Matrix4(
				//	1, 0, 0, 0,
				//	0, 1.33, 0, 0,
				//	0, 0, 1, 0,
				//	0, 0, 0, 1);

				//Ogre::Matrix4 postScalingTranslateMatrix = Ogre::Matrix4(
				//	1, 0, 0, 0,
				//	0, 1.0, 0, 200,
				//	0, 0, 1, 0,
				//	0, 0, 0, 1);

				//mRefractionCamera->setCustomViewMatrix(true, refractionViewMatrix * postScalingTranslateMatrix * scalingMatrix * preScalingTranslateMatrix);
				//mRefractionDepthCamera->setCustomViewMatrix(true, refractionViewMatrix * postScalingTranslateMatrix * scalingMatrix * preScalingTranslateMatrix);
			}
		}
	}

	Ogre::Vector4 Water::getWaterFogColor()
	{
		return mWaterFogColor;
	}

	void Water::setWaterFogColor(Ogre::Vector4 waterFogColor)
	{
		mWaterFogColor = waterFogColor;
		for (unsigned int i = 0; i < mWaterPlaneEntity->getNumSubEntities(); i++)
		{
			mWaterPlaneEntity->getSubEntity(i)->setCustomParameter(0, mWaterFogColor);
		}
	}

	Ogre::Vector4 Water::getMaterialVariables()
	{
		return mMaterialVariables;
	}

	void Water::setMaterialVariables(Ogre::Vector4 materialVariables)
	{
		mMaterialVariables = materialVariables;
		for (unsigned int i = 0; i < mWaterPlaneEntity->getNumSubEntities(); i++)
		{
			mWaterPlaneEntity->getSubEntity(i)->setCustomParameter(1, mMaterialVariables);
		}
	}

	void Water::preRenderTargetUpdate(const Ogre::RenderTargetEvent &evt)
	{
		Ogre::RenderTarget * target = evt.source;

		if (target == mReflectionTexture ||
			target == mReflectionDepthTexture ||
			target == mRefractionTexture ||
			target == mRefractionDepthTexture)
		{
			mWaterPlaneEntity->setVisible(false);
		}

		if (target == mReflectionDepthTexture ||
			target == mRefractionDepthTexture ||
			target == mSceneDepthTexture)
		{
			mInDepthPass = true;
		}

		mInRenderTextureUpdate = true;
	}

	void Water::postRenderTargetUpdate(const Ogre::RenderTargetEvent &evt)
	{
		Ogre::RenderTarget * target = evt.source;

		mInRenderTextureUpdate = false;

		if (target == mReflectionTexture ||
			target == mReflectionDepthTexture ||
			target == mRefractionTexture ||
			target == mRefractionDepthTexture)
		{

			mWaterPlaneEntity->setVisible(true);
		}

		if (target == mReflectionDepthTexture ||
			target == mRefractionDepthTexture ||
			target == mSceneDepthTexture)
		{
			mInDepthPass = false;
		}
	}

	void Water::notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat)
	{
		if (Ogre::StringUtil::endsWith(mat->getName(), "OgreWater/PostProcessingFog", false))
		{
			Ogre::GpuProgramParametersSharedPtr fragmentParams = mat->getBestTechnique()->getPass(0)->getFragmentProgramParameters();
			fragmentParams->setNamedConstant("waterFogColor", mWaterFogColor);
			fragmentParams->setNamedConstant("materialVariables", mMaterialVariables);
		}
		else if (Ogre::StringUtil::endsWith(mat->getName(), "OgreWater/PostProcessingHorizontalBlur", false) ||
			Ogre::StringUtil::endsWith(mat->getName(), "OgreWater/PostProcessingVerticalBlur", false))
		{
			Ogre::GpuProgramParametersSharedPtr fragmentParams = mat->getBestTechnique()->getPass(0)->getFragmentProgramParameters();
			fragmentParams->setNamedConstant("viewportSize", Ogre::Vector4(1.0/mWindow->getWidth(), 1.0/mWindow->getHeight(), 0.0, 0.0));
		}
	}

	bool Water::renderableQueued(Ogre::Renderable *rend, Ogre::uint8 groupID, Ogre::ushort priority, Ogre::Technique **ppTech, Ogre::RenderQueue *pQueue)
	{
		if (mInDepthPass)
		{
			if (rend->getMaterial()->getName().compare("OgreWater/WaterDustMaterial") == 0)
			{
				*ppTech = mWaterDustDepthTechnique;
			}
			else
			{
				*ppTech = mDepthTechnique;
			}
		}

		return true;
	}

	void Water::_writeTexturesToFile()
	{
		mReflectionTexture->writeContentsToFile("Reflection.bmp");
		mReflectionDepthTexture->writeContentsToFile("ReflectionDepth.bmp");
		mRefractionTexture->writeContentsToFile("Refraction.bmp");
		mRefractionDepthTexture->writeContentsToFile("RefractionDepth.bmp");
		mSceneDepthTexture->writeContentsToFile("SceneDepth.bmp");
	}

	void Water::setWaterDustEnabled(bool enable)
	{
		if (enable && !mWaterDustEnabled)
		{
			createWaterDust();
			mWaterDustEnabled = true;
		}
		else if (!enable && mWaterDustEnabled)
		{
			destroyWaterDust();
			mWaterDustEnabled = false;
		}
	}

	void Water::createWaterDust()
	{
		// Set up particle system for "water dust"
		mWaterDustParticleSystem = mSceneMgr->createParticleSystem("WaterDust", "OgreWater/WaterDustParticle");
		//((Ogre::BillboardParticleRenderer *)mWaterDustParticleSystem->getRenderer())->setPointRenderingEnabled(true);
		mWaterDustSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(mCamera->getDerivedPosition());
		mWaterDustSceneNode->attachObject(mWaterDustParticleSystem);
	}

	void Water::destroyWaterDust()
	{
		mWaterDustSceneNode->detachObject(mWaterDustParticleSystem);
		mSceneMgr->destroyParticleSystem(mWaterDustParticleSystem);
		mWaterDustParticleSystem = 0;
		mWaterDustSceneNode->getParentSceneNode()->removeChild(mWaterDustSceneNode);
		mWaterDustSceneNode = 0;
	}

	void Water::setAirBubblesEnabled(bool enable)
	{
		if (enable && !mAirBubblesEnabled)
		{
			createAirBubbles();
			mAirBubblesEnabled = true;
		}
		else if (!enable && mAirBubblesEnabled)
		{
			destroyAirBubbles();
			mAirBubblesEnabled = false;
		}
	}

	void Water::createAirBubbles()
	{
		// Set up particle system for air bubbles
		mAirBubblesParticleSystem = mSceneMgr->createParticleSystem("AirBubbles", "OgreWater/AirBubblesParticle");
		//((Ogre::BillboardParticleRenderer *)mAirBubblesParticleSystem->getRenderer())->setPointRenderingEnabled(true);
		mAirBubblesSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(mCamera->getDerivedPosition());
		mAirBubblesSceneNode->attachObject(mAirBubblesParticleSystem);
	}

	void Water::destroyAirBubbles()
	{
		mAirBubblesSceneNode->detachObject(mAirBubblesParticleSystem);
		mSceneMgr->destroyParticleSystem(mAirBubblesParticleSystem);
		mAirBubblesParticleSystem = 0;
		mAirBubblesSceneNode->getParentSceneNode()->removeChild(mAirBubblesSceneNode);
		mAirBubblesSceneNode = 0;
	}
}