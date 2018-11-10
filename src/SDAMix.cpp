#include "SDAMix.h"

#include "cinder/gl/Texture.h"
#include "cinder/Xml.h"

using namespace ci;
using namespace ci::app;

namespace SophiaDigitalArt {

	SDAMix::SDAMix(SDASettingsRef aSDASettings, SDAAnimationRef aSDAAnimation)
		: mFlipV(false)
		, mFlipH(false)
	{
		CI_LOG_V("SDAMix readSettings");
		// Settings
		mSDASettings = aSDASettings;
		// Animation
		mSDAAnimation = aSDAAnimation;

		fboFmt.setColorTextureFormat(fmt);
		// initialize the textures list with audio texture
		mTexturesFilepath = getAssetPath("") / mSDASettings->mAssetsPath / "textures.xml";
		initTextureList();

		// initialize the shaders list 
		initShaderList();
		mMixesFilepath = getAssetPath("") / "mixes.xml";
		/*if (fs::exists(mMixesFilepath)) {
			// load textures from file if one exists
			// TODO readSettings(mSDASettings, mSDAAnimation, loadFile(mMixesFilepath));
			}*/
			// render fbo
		mRenderFbo = gl::Fbo::create(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight, fboFmt);
	
		mCurrentBlend = 0;
		for (size_t i = 0; i < mSDAAnimation->getBlendModesCount(); i++)
		{
			mBlendFbos[i] = gl::Fbo::create(mSDASettings->mPreviewFboWidth, mSDASettings->mPreviewFboHeight, fboFmt);
		}

		mGlslMix = gl::GlslProg::create(mSDASettings->getDefaultVextexShaderString(), mSDASettings->getMixFragmentShaderString());
		// 20161209 problem on Mac mGlslMix->setLabel("mixfbo");
		mGlslBlend = gl::GlslProg::create(mSDASettings->getDefaultVextexShaderString(), mSDASettings->getMixFragmentShaderString());
		// 20161209 problem on Mac mGlslBlend->setLabel("blend mixfbo");
		// feedback
		mGlslFeedback = gl::GlslProg::create(mSDASettings->getDefaultVextexShaderString(), mSDASettings->getDefaultFragmentShaderString());
		mCurrentFeedbackIndex = 0;
		mFeedbackFrames = 0;
		mFeedbackFbo = gl::Fbo::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight, fboFmt);

		// init textures
		for (size_t i = 0; i < 10; i++)
		{
			mOutputTextures[i] = ci::gl::Texture::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight);
		}
		// shared output
		mSharedOutputActive = false;
		mSharedFboIndex = 0;
		mSpoutInitialized = false;
		strcpy(mSenderName, "Reymenta Spout Sender"); // we have to set a sender name first
	}

#pragma region blendmodes
	unsigned int SDAMix::getFboBlendCount() {
		return mBlendFbos.size();
	}
	ci::gl::TextureRef SDAMix::getFboThumb(unsigned int aBlendIndex) {
		//if (aBlendIndex > mBlendFbos.size() - 1) aBlendIndex = mBlendFbos.size() - 1;
		return mBlendFbos[aBlendIndex]->getColorTexture();
	}
	void SDAMix::useBlendmode(unsigned int aBlendIndex) {
		//if (aBlendIndex > mBlendFbos.size() - 1) aBlendIndex = 0;
		mSDASettings->iBlendmode = aBlendIndex;
	}
	ci::gl::TextureRef SDAMix::getMixTexture(unsigned int aMixFboIndex) {
		if (aMixFboIndex > mMixFbos.size() - 1) aMixFboIndex = 0;
		if (!mMixFbos[aMixFboIndex].texture) {
			// should never happen 
			mMixFbos[aMixFboIndex].texture = gl::Texture2d::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight);
		}
		if (!mMixFbos[aMixFboIndex].fbo) {
			// should never happen
			mMixFbos[aMixFboIndex].fbo = gl::Fbo::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight, fboFmt);
		}
		// texturing sharing
		if (mSharedOutputActive && mSharedFboIndex == aMixFboIndex) {
#if defined( CINDER_MSW )
			// spout
			if (mSpoutInitialized) {
				mSpoutSender.SendTexture(mMixFbos[mSharedFboIndex].texture->getId(), mMixFbos[mSharedFboIndex].texture->getTarget(), mSDASettings->mFboWidth, mSDASettings->mFboHeight);
			}
#endif
#if defined( CINDER_MAC )
			// syphon
			mSyphonServer.publishTexture(mMixFbos[mSharedFboIndex].texture);
#endif
		}
		return mMixFbos[aMixFboIndex].texture;
	}
	// spout output
	void SDAMix::toggleSharedOutput(unsigned int aMixFboIndex) {
		if (aMixFboIndex < mMixFbos.size()) {
			mSharedFboIndex = aMixFboIndex;
		}
		mSharedOutputActive = !mSharedOutputActive;
#if defined( CINDER_MSW )
		if (mSharedOutputActive && !mSpoutInitialized) {
			// Initialize a sender
			mSpoutInitialized = mSpoutSender.CreateSender(mSenderName, mSDASettings->mFboWidth, mSDASettings->mFboHeight);
		}
#endif
	}

	ci::gl::TextureRef SDAMix::getFboTexture(unsigned int aFboIndex) {
		if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
		return mFboList[aFboIndex]->getFboTexture();
	}
	ci::gl::TextureRef SDAMix::getFboRenderedTexture(unsigned int aFboIndex) {
		if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
		return mFboList[aFboIndex]->getRenderedTexture();
	}

	void SDAMix::renderBlend()
	{
		if (mCurrentBlend > mBlendFbos.size() - 1) mCurrentBlend = 0;
		gl::ScopedFramebuffer scopedFbo(mBlendFbos[mCurrentBlend]);
		gl::clear(Color::black());
		// texture binding must be before ScopedGlslProg
		mFboList[0]->getRenderedTexture()->bind(0);
		mFboList[1]->getRenderedTexture()->bind(1);
		gl::ScopedGlslProg glslScope(mGlslBlend);
		gl::drawSolidRect(Rectf(0, 0, mBlendFbos[mCurrentBlend]->getWidth(), mBlendFbos[mCurrentBlend]->getHeight()));
	}
#pragma endregion blendmodes

#pragma region warps
	void SDAMix::save()
	{

	}
	void SDAMix::load()
	{
		CI_LOG_V("SDAMix load: ");
			CI_LOG_V("mMixFbos.size() < mWarps.size(), we create a new mixFbo");
			mMixFbos[0].fbo = gl::Fbo::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight, fboFmt);
			mMixFbos[0].texture = gl::Texture2d::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight);
			mMixFbos[0].name = "new";
	}

	// Render the scene into the FBO
	ci::gl::Texture2dRef SDAMix::getRenderTexture()
	{
		gl::ScopedFramebuffer fbScp(mRenderFbo);
		gl::clear(Color::black());
		// setup the viewport to match the dimensions of the FBO
		gl::ScopedViewport scpVp(ivec2(0), mRenderFbo->getSize());

				// iterate over the warps and draw their content
				/*int i = 0;
				for (auto &warp : mWarps) {
					//warp->draw(mMixes[0]->getMixTexture(mWarpFboIndex), Area(0, 0, mMixes[0]->getFboTextureWidth(mWarpFboIndex), mMixes[0]->getFboTextureHeight(mWarpFboIndex)));
					if (warp->isActive()) warp->draw(getMixTexture(i), getMixTexture(i)->getBounds());
					i++;
				}*/
		// feedback
		if (mFeedbackFrames > 0) {
			mCurrentFeedbackIndex++;
			if (mCurrentFeedbackIndex > mFeedbackFrames) mCurrentFeedbackIndex = 0;
			// save rendered texture at mCurrentFeedbackIndex
			Surface s8(mRenderedTexture->createSource());
			mOutputTextures[mCurrentFeedbackIndex] = ci::gl::Texture::create(s8);
			mGlslFeedback->uniform("iResolution", vec3(mSDAAnimation->getFloatUniformValueByName("iResolutionX"), mSDAAnimation->getFloatUniformValueByName("iResolutionY"), 1.0));
			mGlslFeedback->uniform("iChannel0", 0);
			mGlslFeedback->uniform("iChannel1", 1);
			mGlslFeedback->uniform("iChannel2", 2);
			mGlslFeedback->uniform("iChannel3", 3);
			mGlslFeedback->uniform("iChannel4", 4);
			mGlslFeedback->uniform("iChannel5", 5);
			mGlslFeedback->uniform("iChannel6", 6);
			mGlslFeedback->uniform("iChannel7", 7);
			mGlslFeedback->uniform("iChannel8", 8);
			mGlslFeedback->uniform("iChannel9", 9);

			gl::ScopedFramebuffer fbScp(mFeedbackFbo);
			gl::clear(Color::black());

			mOutputTextures[0]->bind(0);
			mOutputTextures[1]->bind(1);
			mOutputTextures[2]->bind(2);
			mOutputTextures[3]->bind(3);
			mOutputTextures[4]->bind(4);
			mOutputTextures[5]->bind(5);
			mOutputTextures[6]->bind(6);
			mOutputTextures[7]->bind(7);
			mOutputTextures[8]->bind(8);
			mOutputTextures[9]->bind(9);

			gl::ScopedGlslProg glslScope(mGlslFeedback);
			gl::drawSolidRect(Rectf(0, 0, mSDASettings->mFboWidth, mSDASettings->mFboHeight));

			mFeedbackTexture = mFeedbackFbo->getColorTexture();
			/*string filename = toString(mCurrentFeedbackIndex) + ".jpg";
			if (getElapsedFrames() % 100 == 0) {
			writeImage(writeFile(getAssetPath("") / "output" / filename), s8);
			}
			if (getElapsedFrames() % 104 == 0) {
			Surface sk8(mOutputTextures[mCurrentFeedbackIndex]->createSource());
			writeImage(writeFile(getAssetPath("") / "output" / "1" / filename), sk8);
			}
			if (getElapsedFrames() % 108 == 0) {
			Surface skr8(mFeedbackTexture->createSource());
			writeImage(writeFile(getAssetPath("") / "output" / "2" / filename), skr8);
			}*/
			return mFeedbackTexture;
		}

		mRenderedTexture = mRenderFbo->getColorTexture();
		return mRenderedTexture;
	}
	
#pragma endregion warps
	void SDAMix::renderMix() {
		if (mFboList.size() > 0) {
			if (!mMixFbos[0].fbo) mMixFbos[0].fbo = gl::Fbo::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight, fboFmt);
			gl::ScopedFramebuffer scopedFbo(mMixFbos[0].fbo);
			gl::clear(Color::black());
			// render A and B fbos 
			//CI_LOG_V(" iCrossfade " + toString(mWarps[warpMixToRender]->ABCrossfade) + " getAFboIndex " + toString(mWarps[warpMixToRender]->getAFboIndex()) + " getBFboIndex " + toString(mWarps[warpMixToRender]->getBFboIndex()));
			/*mFboList[mWarps[warpMixToRender]->getAFboIndex()]->getFboTexture();
			mFboList[mWarps[warpMixToRender]->getBFboIndex()]->getFboTexture();
			// texture binding must be before ScopedGlslProg
			mFboList[mWarps[warpMixToRender]->getAFboIndex()]->getRenderedTexture()->bind(0);
			mFboList[mWarps[warpMixToRender]->getBFboIndex()]->getRenderedTexture()->bind(1);*/
			mFboList[0]->getFboTexture();
			mFboList[1]->getFboTexture();
			// texture binding must be before ScopedGlslProg
			mFboList[0]->getRenderedTexture()->bind(0);
			mFboList[1]->getRenderedTexture()->bind(1);
			gl::ScopedGlslProg glslScope(mGlslMix);
			mGlslMix->uniform("iCrossfade", mSDASettings->xFade);

			gl::drawSolidRect(Rectf(0, 0, mMixFbos[0].fbo->getWidth(), mMixFbos[0].fbo->getHeight()));

			// save to a texture
			mMixFbos[0].texture = mMixFbos[0].fbo->getColorTexture();
		}		
	}

	string SDAMix::getMixFboName(unsigned int aMixFboIndex) {
		if (aMixFboIndex > mMixFbos.size() - 1) aMixFboIndex = mMixFbos.size() - 1;
		mMixFbos[aMixFboIndex].name = mFboList[0]->getShaderName() + "/" + mFboList[1]->getShaderName();
		return mMixFbos[aMixFboIndex].name;
	}
	void SDAMix::resize() {
		mRenderFbo = gl::Fbo::create(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight, fboFmt);
	}
	void SDAMix::updateAudio() {
		mTextureList[0]->getTexture();
	}
	void SDAMix::update() {
		// update audio texture
		if (mSDASettings->mUseAudio) updateAudio();	

		// check if xFade changed
		if (mSDASettings->xFadeChanged) {
			mSDASettings->xFadeChanged = false;
			
		}
		//vec4 mouse = mSDAAnimation->getVec4UniformValueByName("iMouse");
		mGlslMix->uniform("iWeight0", mSDAAnimation->getFloatUniformValueByIndex(35));	// weight of channel 0
		mGlslMix->uniform("iWeight1", mSDAAnimation->getFloatUniformValueByIndex(36));	// weight of channel 1
		mGlslMix->uniform("iWeight2", mSDAAnimation->getFloatUniformValueByIndex(37));	// weight of channel 2
   
		mGlslMix->uniform("iBlendmode", mSDASettings->iBlendmode);
		mGlslMix->uniform("iGlobalTime", mSDAAnimation->getFloatUniformValueByIndex(0));
		// was vec3(mSDASettings->mFboWidth, mSDASettings->mFboHeight, 1.0)):
		mGlslMix->uniform("iResolution", vec3(mSDAAnimation->getFloatUniformValueByName("iResolutionX"), mSDAAnimation->getFloatUniformValueByName("iResolutionY"), 1.0));
		//mGlslMix->uniform("iChannelResolution", mSDASettings->iChannelResolution, 4);
		// 20180318 mGlslMix->uniform("iMouse", mSDAAnimation->getVec4UniformValueByName("iMouse"));
		mGlslMix->uniform("iMouse", vec3(mSDAAnimation->getFloatUniformValueByIndex(35), mSDAAnimation->getFloatUniformValueByIndex(36), mSDAAnimation->getFloatUniformValueByIndex(37)));
		mGlslMix->uniform("iDate", mSDAAnimation->getVec4UniformValueByName("iDate"));
		mGlslMix->uniform("iChannel0", 0);
		mGlslMix->uniform("iChannel1", 1);
		mGlslMix->uniform("iChannel2", 2);
		mGlslMix->uniform("iRatio", mSDAAnimation->getFloatUniformValueByIndex(11));//check if needed: +1;//mSDASettings->iRatio);
		mGlslMix->uniform("iRenderXY", mSDASettings->mRenderXY);
		mGlslMix->uniform("iZoom", mSDAAnimation->getFloatUniformValueByIndex(12));
		mGlslMix->uniform("iAlpha", mSDAAnimation->getFloatUniformValueByIndex(4) * mSDASettings->iAlpha);
		mGlslMix->uniform("iChromatic", mSDAAnimation->getFloatUniformValueByIndex(17));
		mGlslMix->uniform("iRotationSpeed", mSDAAnimation->getFloatUniformValueByIndex(9));
		mGlslMix->uniform("iCrossfade", mSDASettings->xFade);
		mGlslMix->uniform("iPixelate", mSDAAnimation->getFloatUniformValueByIndex(15));
		mGlslMix->uniform("iExposure", mSDAAnimation->getFloatUniformValueByIndex(14));
		mGlslMix->uniform("iToggle", (int)mSDAAnimation->getBoolUniformValueByIndex(46));
		mGlslMix->uniform("iGreyScale", (int)mSDASettings->iGreyScale);
		mGlslMix->uniform("iBackgroundColor", mSDAAnimation->getVec3UniformValueByName("iBackgroundColor"));// vec3(mSDAAnimation->getFloatUniformValueByIndex(5), mSDAAnimation->getFloatUniformValueByIndex(6), mSDAAnimation->getFloatUniformValueByIndex(7)));
		mGlslMix->uniform("iVignette", (int)mSDAAnimation->getBoolUniformValueByIndex(47));
		mGlslMix->uniform("iInvert", (int)mSDAAnimation->getBoolUniformValueByIndex(48));
		mGlslMix->uniform("iTempoTime", mSDAAnimation->getFloatUniformValueByName("iTempoTime"));
		mGlslMix->uniform("iGlitch", (int)mSDAAnimation->getBoolUniformValueByIndex(45));
		mGlslMix->uniform("iTrixels", mSDAAnimation->getFloatUniformValueByIndex(16));
		mGlslMix->uniform("iRedMultiplier", mSDAAnimation->getFloatUniformValueByName("iRedMultiplier"));
		mGlslMix->uniform("iGreenMultiplier", mSDAAnimation->getFloatUniformValueByName("iGreenMultiplier"));
		mGlslMix->uniform("iBlueMultiplier", mSDAAnimation->getFloatUniformValueByName("iBlueMultiplier"));
		mGlslMix->uniform("iFlipV", mSDAAnimation->isFlipV());
		mGlslMix->uniform("iFlipH", mSDAAnimation->isFlipH());
		mGlslMix->uniform("iParam1", mSDASettings->iParam1);
		mGlslMix->uniform("iParam2", mSDASettings->iParam2);
		mGlslMix->uniform("iXorY", mSDASettings->iXorY);
		mGlslMix->uniform("iBadTv", mSDAAnimation->getFloatUniformValueByName("iBadTv"));
		mGlslMix->uniform("iFps", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IFPS));
		mGlslMix->uniform("iContour", mSDAAnimation->getFloatUniformValueByName("iContour"));
		
		renderMix();
		

		// blendmodes preview
		if (mSDAAnimation->renderBlend()) {
			mCurrentBlend = getElapsedFrames() % mSDAAnimation->getBlendModesCount();
			mGlslBlend->uniform("iBlendmode", mCurrentBlend);
			mGlslBlend->uniform("iGlobalTime", mSDAAnimation->getFloatUniformValueByIndex(0));
			mGlslBlend->uniform("iResolution", vec3(mSDASettings->mPreviewFboWidth, mSDASettings->mPreviewFboHeight, 1.0));
			//mGlslBlend->uniform("iChannelResolution", mSDASettings->iChannelResolution, 4);
			// 20180318 mGlslBlend->uniform("iMouse", mSDAAnimation->getVec4UniformValueByName("iMouse"));
			mGlslBlend->uniform("iMouse", vec3(mSDAAnimation->getFloatUniformValueByIndex(35), mSDAAnimation->getFloatUniformValueByIndex(36), mSDAAnimation->getFloatUniformValueByIndex(37)));
			mGlslBlend->uniform("iDate", mSDAAnimation->getVec4UniformValueByName("iDate"));
			mGlslBlend->uniform("iChannel0", 0);
			mGlslBlend->uniform("iChannel1", 1);
			mGlslBlend->uniform("iAudio0", 0);
			mGlslBlend->uniform("iFreq0", mSDAAnimation->getFloatUniformValueByName("iFreq0"));
			mGlslBlend->uniform("iFreq1", mSDAAnimation->getFloatUniformValueByName("iFreq1"));
			mGlslBlend->uniform("iFreq2", mSDAAnimation->getFloatUniformValueByName("iFreq2"));
			mGlslBlend->uniform("iFreq3", mSDAAnimation->getFloatUniformValueByName("iFreq3"));
			mGlslBlend->uniform("iChannelTime", mSDASettings->iChannelTime, 4);
			mGlslBlend->uniform("iColor", vec3(mSDAAnimation->getFloatUniformValueByIndex(1), mSDAAnimation->getFloatUniformValueByIndex(2), mSDAAnimation->getFloatUniformValueByIndex(3)));
			mGlslBlend->uniform("iBackgroundColor", mSDAAnimation->getVec3UniformValueByName("iBackgroundColor"));//vec3(mSDAAnimation->getFloatUniformValueByIndex(5), mSDAAnimation->getFloatUniformValueByIndex(6), mSDAAnimation->getFloatUniformValueByIndex(7)));
			mGlslBlend->uniform("iSteps", (int)mSDAAnimation->getFloatUniformValueByIndex(10));
			mGlslBlend->uniform("iRatio", mSDAAnimation->getFloatUniformValueByIndex(11));
			mGlslBlend->uniform("width", 1);
			mGlslBlend->uniform("height", 1);
			mGlslBlend->uniform("iRenderXY", mSDASettings->mRenderXY);
			mGlslBlend->uniform("iZoom", mSDAAnimation->getFloatUniformValueByIndex(12));
			mGlslBlend->uniform("iAlpha", mSDAAnimation->getFloatUniformValueByIndex(4) * mSDASettings->iAlpha);
			mGlslBlend->uniform("iChromatic", mSDAAnimation->getFloatUniformValueByIndex(17));
			mGlslBlend->uniform("iRotationSpeed", mSDAAnimation->getFloatUniformValueByIndex(9));
			mGlslBlend->uniform("iCrossfade", 0.5f);// blendmode only work if different than 0 or 1.0 mSDAAnimation->getFloatUniformValueByIndex(18]);
			mGlslBlend->uniform("iPixelate", mSDAAnimation->getFloatUniformValueByIndex(15));
			mGlslBlend->uniform("iExposure", mSDAAnimation->getFloatUniformValueByIndex(14));
			mGlslBlend->uniform("iDeltaTime", mSDAAnimation->iDeltaTime);
			mGlslBlend->uniform("iFade", (int)mSDASettings->iFade);
			mGlslBlend->uniform("iToggle", (int)mSDAAnimation->getBoolUniformValueByIndex(46));
			mGlslBlend->uniform("iGreyScale", (int)mSDASettings->iGreyScale);
			mGlslBlend->uniform("iTransition", mSDASettings->iTransition);
			mGlslBlend->uniform("iAnim", mSDASettings->iAnim.value());
			mGlslBlend->uniform("iRepeat", (int)mSDASettings->iRepeat);
			mGlslBlend->uniform("iVignette", (int)mSDAAnimation->getBoolUniformValueByIndex(47));
			mGlslBlend->uniform("iInvert", (int)mSDAAnimation->getBoolUniformValueByIndex(48));
			mGlslBlend->uniform("iDebug", (int)mSDASettings->iDebug);
			mGlslBlend->uniform("iShowFps", (int)mSDASettings->iShowFps);
			mGlslBlend->uniform("iFps", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IFPS));
			mGlslBlend->uniform("iTempoTime", mSDAAnimation->getFloatUniformValueByName("iTempoTime"));
			mGlslBlend->uniform("iGlitch", (int)mSDAAnimation->getBoolUniformValueByIndex(45));
			mGlslBlend->uniform("iTrixels", mSDAAnimation->getFloatUniformValueByIndex(16));
			//mGlslBlend->uniform("iPhase", mSDASettings->iPhase);
			mGlslBlend->uniform("iSeed", mSDASettings->iSeed);
			mGlslBlend->uniform("iRedMultiplier", mSDAAnimation->getFloatUniformValueByName("iRedMultiplier"));
			mGlslBlend->uniform("iGreenMultiplier", mSDAAnimation->getFloatUniformValueByName("iGreenMultiplier"));
			mGlslBlend->uniform("iBlueMultiplier", mSDAAnimation->getFloatUniformValueByName("iBlueMultiplier"));
			mGlslBlend->uniform("iFlipH", 0);
			mGlslBlend->uniform("iFlipV", 0);
			mGlslBlend->uniform("iParam1", mSDASettings->iParam1);
			mGlslBlend->uniform("iParam2", mSDASettings->iParam2);
			mGlslBlend->uniform("iXorY", mSDASettings->iXorY);
			mGlslBlend->uniform("iBadTv", mSDAAnimation->getFloatUniformValueByName("iBadTv"));
			mGlslBlend->uniform("iContour", mSDAAnimation->getFloatUniformValueByName("iContour"));
			renderBlend();
		}
	}
#pragma region events

	bool SDAMix::handleMouseMove(MouseEvent &event)
	{
		bool handled = false;
		
		event.setHandled(handled);
		return event.isHandled();
	}

	bool SDAMix::handleMouseDown(MouseEvent &event)
	{
		bool handled = false;
		
		event.setHandled(handled);
		return event.isHandled();
	}
	bool SDAMix::handleMouseDrag(MouseEvent &event)
	{
		bool handled = false;
		
		event.setHandled(handled);
		return event.isHandled();
	}
	bool SDAMix::handleMouseUp(MouseEvent &event)
	{
		bool handled = false;
		
		event.setHandled(handled);
		return event.isHandled();
	}

	bool SDAMix::handleKeyDown(KeyEvent &event)
	{
		bool handled = false;

		event.setHandled(handled);
		return event.isHandled();
	}
	bool SDAMix::handleKeyUp(KeyEvent &event)
	{
		bool handled = false;
		
		event.setHandled(handled);
		return event.isHandled();
	}
#pragma endregion events
	void SDAMix::setFboInputTexture(unsigned int aFboIndex, unsigned int aInputTextureIndex) {
		if (aFboIndex > mFboList.size() - 1) aFboIndex = mFboList.size() - 1;
		if (aInputTextureIndex > mTextureList.size() - 1) aInputTextureIndex = mTextureList.size() - 1;
		mFboList[aFboIndex]->setInputTexture(mTextureList, aInputTextureIndex);
	}
	unsigned int SDAMix::getFboInputTextureIndex(unsigned int aFboIndex) {
		if (aFboIndex > mFboList.size() - 1) aFboIndex = mFboList.size() - 1;
		return mFboList[aFboIndex]->getInputTextureIndex();
	}
	void SDAMix::initShaderList() {

		if (mShaderList.size() == 0) {
			CI_LOG_V("SDASession::init mShaderList");
			createShaderFboFromString("void main(void){vec2 uv = gl_FragCoord.xy / iResolution.xy;fragColor = vec4(sin(uv.x), sin(uv.y), 0.0, 1.0);}", "tex1");
			createShaderFboFromString("void main(void){vec2 uv = gl_FragCoord.xy / iResolution.xy;fragColor = texture(iChannel0, uv);}", "tex0");
			createShaderFboFromString("void main(void){vec2 uv = gl_FragCoord.xy / iResolution.xy;fragColor = texture(iChannel0, uv);}", "tex1");
		}
	}
	bool SDAMix::initTextureList() {
		bool isFirstLaunch = false;
		if (mTextureList.size() == 0) {
			CI_LOG_V("SDASession::init mTextureList");
			isFirstLaunch = true;
			// add an audio texture as first texture
			TextureAudioRef t(new TextureAudio(mSDAAnimation));

			// add texture xml
			XmlTree			textureXml;
			textureXml.setTag("texture");
			textureXml.setAttribute("id", "0");
			textureXml.setAttribute("texturetype", "audio");

			t->fromXml(textureXml);
			mTextureList.push_back(t);
			// then read textures.xml
			if (fs::exists(mTexturesFilepath)) {
				// load textures from file if one exists
				//mTextureList = SDATexture::readSettings(mSDAAnimation, loadFile(mTexturesFilepath));
				XmlTree			doc;
				try { doc = XmlTree(loadFile(mTexturesFilepath)); }
				catch (...) { CI_LOG_V("could not load textures.xml"); }
				if (doc.hasChild("textures")) {
					XmlTree xml = doc.getChild("textures");
					for (XmlTree::ConstIter textureChild = xml.begin("texture"); textureChild != xml.end(); ++textureChild) {
						CI_LOG_V("texture ");

						string texturetype = textureChild->getAttributeValue<string>("texturetype", "unknown");
						CI_LOG_V("texturetype " + texturetype);
						XmlTree detailsXml = textureChild->getChild("details");
						// read or add the assets path
						string mFolder = detailsXml.getAttributeValue<string>("folder", "");
						if (mFolder.length() == 0) detailsXml.setAttribute("folder", mSDASettings->mAssetsPath);
						// create the texture
						if (texturetype == "image") {
							TextureImageRef t(TextureImage::create());
							t->fromXml(detailsXml);
							mTextureList.push_back(t);
						}
						else if (texturetype == "imagesequence") {
							TextureImageSequenceRef t(new TextureImageSequence(mSDAAnimation));
							t->fromXml(detailsXml);
							mTextureList.push_back(t);
						}
						else if (texturetype == "camera") {
#if (defined(  CINDER_MSW) ) || (defined( CINDER_MAC ))
							TextureCameraRef t(new TextureCamera());
							t->fromXml(detailsXml);
							mTextureList.push_back(t);
#else
							// camera not supported on this platform
							CI_LOG_V("camera not supported on this platform");
							XmlTree		xml;
							xml.setTag("details");
							xml.setAttribute("path", "0.jpg");
							xml.setAttribute("width", 640);
							xml.setAttribute("height", 480);
							t->fromXml(xml);
							mTextureList.push_back(t);
#endif
						}
						else if (texturetype == "shared") {
							// TODO CHECK USELESS? #if defined( CINDER_MSW )
							TextureSharedRef t(new TextureShared());
							t->fromXml(detailsXml);
							mTextureList.push_back(t);
							//#endif
						}
						else if (texturetype == "audio") {
							// audio texture done in initTextures
						}
						else if (texturetype == "stream") {
							// stream texture done when websocket texture received
						}
						else {
							// unknown texture type
							CI_LOG_V("unknown texture type");
							TextureImageRef t(new TextureImage());
							XmlTree		xml;
							xml.setTag("details");
							xml.setAttribute("path", "0.jpg");
							xml.setAttribute("width", 640);
							xml.setAttribute("height", 480);
							t->fromXml(xml);
							mTextureList.push_back(t);
						}
					}
				}
			}
		}
		return isFirstLaunch;
	}
	void SDAMix::fboFlipV(unsigned int aFboIndex) {
		if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
		mFboList[aFboIndex]->flipV();
	}
	bool SDAMix::isFboFlipV(unsigned int aFboIndex) {
		if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
		return mFboList[aFboIndex]->isFlipV();
	}
	void SDAMix::setFboFragmentShaderIndex(unsigned int aFboIndex, unsigned int aFboShaderIndex) {
		CI_LOG_V("setFboFragmentShaderIndex, before, fboIndex: " + toString(aFboIndex) + " shaderIndex " + toString(aFboShaderIndex));
		if (aFboIndex > mFboList.size() - 1) aFboIndex = mFboList.size() - 1;
		if (aFboShaderIndex > mShaderList.size() - 1) aFboShaderIndex = mShaderList.size() - 1;
		CI_LOG_V("setFboFragmentShaderIndex, after, fboIndex: " + toString(aFboIndex) + " shaderIndex " + toString(aFboShaderIndex));
		mFboList[aFboIndex]->setFragmentShader(aFboShaderIndex, mShaderList[aFboShaderIndex]->getFragmentString(), mShaderList[aFboShaderIndex]->getName());
		// route message
		// LOOP! mSDAWebsocket->changeFragmentShader(mShaderList[aFboShaderIndex]->getFragmentString());
	}
	unsigned int SDAMix::getFboFragmentShaderIndex(unsigned int aFboIndex) {
		unsigned int rtn = mFboList[aFboIndex]->getShaderIndex();
		//CI_LOG_V("getFboFragmentShaderIndex, fboIndex: " + toString(aFboIndex)+" shaderIndex: " + toString(rtn));
		if (rtn > mShaderList.size() - 1) rtn = mShaderList.size() - 1;
		return rtn;
	}
#pragma region textures
	ci::gl::TextureRef SDAMix::getInputTexture(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getTexture();
	}
	string SDAMix::getInputTextureName(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getName();
	}
	unsigned int SDAMix::getInputTexturesCount() {
		return mTextureList.size();
	}
	unsigned int SDAMix::getInputTextureOriginalWidth(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getOriginalWidth();
	}
	unsigned int SDAMix::getInputTextureOriginalHeight(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getOriginalHeight();
	}
	int SDAMix::getInputTextureXLeft(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getXLeft();
	}
	void SDAMix::setInputTextureXLeft(unsigned int aTextureIndex, int aXLeft) {
		mTextureList[aTextureIndex]->setXLeft(aXLeft);
	}
	int SDAMix::getInputTextureYTop(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getYTop();
	}
	void SDAMix::setInputTextureYTop(unsigned int aTextureIndex, int aYTop) {
		mTextureList[aTextureIndex]->setYTop(aYTop);
	}
	int SDAMix::getInputTextureXRight(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getXRight();
	}
	void SDAMix::setInputTextureXRight(unsigned int aTextureIndex, int aXRight) {
		mTextureList[aTextureIndex]->setXRight(aXRight);
	}
	int SDAMix::getInputTextureYBottom(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getYBottom();
	}
	void SDAMix::setInputTextureYBottom(unsigned int aTextureIndex, int aYBottom) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->setYBottom(aYBottom);
	}
	bool SDAMix::isFlipVInputTexture(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->isFlipV();
	}
	void SDAMix::inputTextureFlipV(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->flipV();
	}
	bool SDAMix::isFlipHInputTexture(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->isFlipH();
	}
	void SDAMix::inputTextureFlipH(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->flipH();
	}

	bool SDAMix::getInputTextureLockBounds(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getLockBounds();
	}
	void SDAMix::toggleInputTextureLockBounds(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->toggleLockBounds();
	}
	void SDAMix::togglePlayPause(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->togglePlayPause();
	}
	bool SDAMix::loadImageSequence(string aFolder, unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		CI_LOG_V("loadImageSequence " + aFolder + " at textureIndex " + toString(aTextureIndex));
		// add texture xml
		XmlTree			textureXml;
		textureXml.setTag("texture");
		textureXml.setAttribute("id", "0");
		textureXml.setAttribute("texturetype", "sequence");
		textureXml.setAttribute("path", aFolder);
		TextureImageSequenceRef t(new TextureImageSequence(mSDAAnimation));
		if (t->fromXml(textureXml)) {
			mTextureList.push_back(t);
			return true;
		}
		else {
			return false;
		}
	}
	void SDAMix::loadMovie(string aFile, unsigned int aTextureIndex) {

	}
	void SDAMix::loadImageFile(string aFile, unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		CI_LOG_V("loadImageFile " + aFile + " at textureIndex " + toString(aTextureIndex));
		mTextureList[aTextureIndex]->loadFromFullPath(aFile);
	}
	void SDAMix::loadAudioFile(string aFile) {
		mTextureList[0]->loadFromFullPath(aFile);
	}
	bool SDAMix::isMovie(unsigned int aTextureIndex) {
		return false;
	}

	// sequence
	bool SDAMix::isSequence(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return (mTextureList[aTextureIndex]->getType() == mTextureList[aTextureIndex]->SEQUENCE);
	}
	bool SDAMix::isLoadingFromDisk(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return (mTextureList[aTextureIndex]->isLoadingFromDisk());
	}
	void SDAMix::toggleLoadingFromDisk(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->toggleLoadingFromDisk();
	}
	void SDAMix::syncToBeat(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->syncToBeat();
	}
	void SDAMix::reverse(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->reverse();
	}
	float SDAMix::getSpeed(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getSpeed();
	}
	void SDAMix::setSpeed(unsigned int aTextureIndex, float aSpeed) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->setSpeed(aSpeed);
	}
	int SDAMix::getPosition(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getPosition();
	}
	void SDAMix::setPlayheadPosition(unsigned int aTextureIndex, int aPosition) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mTextureList[aTextureIndex]->setPlayheadPosition(aPosition);
	}
	int SDAMix::getMaxFrame(unsigned int aTextureIndex) {
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		return mTextureList[aTextureIndex]->getMaxFrame();
	}
#pragma endregion textures
	// shaders
	void SDAMix::updateShaderThumbFile(unsigned int aShaderIndex) {
		for (int i = 0; i < mFboList.size(); i++)
		{
			if (mFboList[i]->getShaderIndex() == aShaderIndex) mFboList[i]->updateThumbFile();
		}
	}
	void SDAMix::removeShader(unsigned int aShaderIndex) {
		if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
		mShaderList[aShaderIndex]->removeShader();
	}
	void SDAMix::setFragmentShaderString(unsigned int aShaderIndex, string aFragmentShaderString, string aName) {
		if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
		mShaderList[aShaderIndex]->setFragmentString(aFragmentShaderString, aName);
		// if live coding shader compiles and is used by a fbo reload it
		for (int i = 0; i < mFboList.size(); i++)
		{
			if (mFboList[i]->getShaderIndex() == aShaderIndex) setFboFragmentShaderIndex(i, aShaderIndex);
		}
	}
	unsigned int SDAMix::createShaderFboFromString(string aFragmentShaderString, string aShaderFilename) {
		unsigned int rtn = 0;
		// create new shader
		SDAShaderRef s(new SDAShader(mSDASettings, mSDAAnimation, aShaderFilename, aFragmentShaderString));
		if (s->isValid()) {
			mShaderList.push_back(s);
			rtn = mShaderList.size() - 1;
			// each shader element has a fbo
			SDAFboRef f(new SDAFbo(mSDASettings, mSDAAnimation));
			// create fbo xml
			XmlTree			fboXml;
			fboXml.setTag(aShaderFilename);
			fboXml.setAttribute("id", rtn);
			fboXml.setAttribute("width", "640");
			fboXml.setAttribute("height", "480");
			fboXml.setAttribute("shadername", mShaderList[rtn]->getName());
			fboXml.setAttribute("inputtextureindex", math<int>::min(rtn, mTextureList.size() - 1));
			f->fromXml(fboXml);
			//f->setShaderIndex(rtn);
			f->setFragmentShader(rtn, mShaderList[rtn]->getFragmentString(), mShaderList[rtn]->getName());
			mFboList.push_back(f);
			setFboInputTexture(mFboList.size() - 1, math<int>::min(rtn, mTextureList.size() - 1));
		}
		return rtn;
	}
	string SDAMix::getFragmentShaderString(unsigned int aShaderIndex) {
		if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
		return mShaderList[aShaderIndex]->getFragmentString();
	}
	/*string SDAMix::getVertexShaderString(unsigned int aShaderIndex) {
		if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
		return mShaderList[aShaderIndex]->getVertexString();
	}*/
	string SDAMix::getShaderName(unsigned int aShaderIndex) {
		if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
		return mShaderList[aShaderIndex]->getName();
	}
	ci::gl::TextureRef SDAMix::getShaderThumb(unsigned int aShaderIndex) {
		unsigned int found = 0;
		for (int i = 0; i < mFboList.size(); i++)
		{
			if (mFboList[i]->getShaderIndex() == aShaderIndex) found = i;
		}
		return getFboRenderedTexture(found);
	}
	void SDAMix::updateStream(string * aStringPtr) {
		int found = -1;
		for (int i = 0; i < mTextureList.size(); i++)
		{
			if (mTextureList[i]->getType() == mTextureList[i]->STREAM) found = i;
		}
		if (found < 0) {
			// create stream texture
			TextureStreamRef t(new TextureStream(mSDAAnimation));
			// add texture xml
			XmlTree			textureXml;
			textureXml.setTag("texture");
			textureXml.setAttribute("id", "9");
			textureXml.setAttribute("texturetype", "stream");
			t->fromXml(textureXml);
			mTextureList.push_back(t);
			found = mTextureList.size() - 1;
		}
		mTextureList[found]->loadFromFullPath(*aStringPtr);
	}


	unsigned int SDAMix::createShaderFbo(string aShaderFilename, unsigned int aInputTextureIndex) {
		// initialize rtn to 0 to force creation
		unsigned int rtn = 0;
		string fName = aShaderFilename;
		if (aShaderFilename.length() > 0) {
			fs::path mFragFile = getAssetPath("") / mSDASettings->mAssetsPath / aShaderFilename;
			if (!fs::exists(mFragFile)) {
				// if file does not exist it may be a full path
				mFragFile = aShaderFilename;
			}
			if (fs::exists(mFragFile)) {
				// check if mShaderList contains a shader
				if (mShaderList.size() > 0) {
					fName = mFragFile.filename().string();
					// find a removed shader
					for (int i = mShaderList.size() - 1; i > 0; i--)
					{
						if (!mShaderList[i]->isValid() || fName == mShaderList[i]->getName()) { rtn = i; }
					}
					// find a not used shader if no removed shader
					if (rtn == 0) {
						// first reset all shaders (excluding the first 8 ones)
						for (int i = mShaderList.size() - 1; i > 8; i--)
						{
							mShaderList[i]->setActive(false);
						}
						
						// find inactive shader index
						for (int i = mShaderList.size() - 1; i > 8; i--)
						{
							if (!mShaderList[i]->isActive()) rtn = i;
						}
					}
				}
				// if we found an available slot
				if (rtn > 0) {
					if (rtn < mFboList.size()) {
						if (mShaderList[rtn]->loadFragmentStringFromFile(aShaderFilename)) {
							mFboList[rtn]->setFragmentShader(rtn, mShaderList[rtn]->getFragmentString(), mShaderList[rtn]->getName());
						}
					}
				}
				else {
					// no slot available, create new shader
					rtn = createShaderFboFromString(loadString(loadFile(mFragFile)), aShaderFilename);
				}
				if (rtn > 0) mFboList[rtn]->updateThumbFile();
			}
		}
		return rtn;
	}
} // namespace SophiaDigitalArt
