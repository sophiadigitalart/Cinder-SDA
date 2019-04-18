//
//  SDAsession.cpp
//

#include "SDASession.h"

using namespace SophiaDigitalArt;

SDASession::SDASession(SDASettingsRef aSDASettings)
{
	CI_LOG_V("SDASession ctor");
	mSDASettings = aSDASettings;
	// allow log to file
	mSDALog = SDALog::create();
	// Utils
	mSDAUtils = SDAUtils::create(mSDASettings);
	// Animation
	mSDAAnimation = SDAAnimation::create(mSDASettings);
	// TODO: needed? mSDAAnimation->tapTempo();
		// allow log to file
	mSDALog = SDALog::create();
	// init fbo format
	//fmt.setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
	//fmt.setBorderColor(Color::black());		
	// uncomment this to enable 4x antialiasing	
	//fboFmt.setSamples( 4 ); 
	fboFmt.setColorTextureFormat(fmt);
	mPosX = mPosY = 0.0f;
	mZoom = 1.0f;
	mSelectedWarp = 0;

	// Websocket
	mSDAWebsocket = SDAWebsocket::create(mSDASettings, mSDAAnimation);
	// Message router
	mSDARouter = SDARouter::create(mSDASettings, mSDAAnimation, mSDAWebsocket);
	// warping
	gl::enableDepthRead();
	gl::enableDepthWrite();

	// reset no matter what, so we don't miss anything
	reset();

	// check to see if SDASession.xml file exists and restore if it does
	sessionPath = getAssetPath("") / mSDASettings->mAssetsPath / sessionFileName;
	if (fs::exists(sessionPath))
	{
		restore();
	}
	else
	{
		// Create json file if it doesn't already exist.
		std::ofstream oStream(sessionPath.string());
		oStream.close();
		save();
	}

	cmd = -1;
	mFreqWSSend = false;
	mEnabledAlphaBlending = true;
	// mix
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

	try
	{
		mGlslMix = gl::GlslProg::create(mSDASettings->getDefaultVextexShaderString(), mSDASettings->getMixFragmentShaderString());
		mGlslBlend = gl::GlslProg::create(mSDASettings->getDefaultVextexShaderString(), mSDASettings->getMixFragmentShaderString());
	}
	catch (gl::GlslProgCompileExc &exc)
	{
		mError = "mix error:" + string(exc.what());
		CI_LOG_V("setFragmentString, unable to compile live fragment shader:" + mError);
	}
	catch (const std::exception &e)
	{
		mError = "mix error:" + string(e.what());
		CI_LOG_V("setFragmentString, error on live fragment shader:" + mError);
	}
	mSDASettings->mMsg = mError;

	mAFboIndex = 0;
	mBFboIndex = 1;
}

SDASessionRef SDASession::create(SDASettingsRef aSDASettings)
{
	return shared_ptr<SDASession>(new SDASession(aSDASettings));
}

void SDASession::readSettings(SDASettingsRef aSDASettings, SDAAnimationRef aSDAAnimation, const DataSourceRef &source) {
	XmlTree			doc;

	CI_LOG_V("SDASession readSettings");
	// try to load the specified xml file
	try {
		doc = XmlTree(source);
		CI_LOG_V("SDASession xml doc ok");
	}
	catch (...) {
		CI_LOG_V("SDASession xml doc error");
	}

	// check if this is a valid file 
	bool isOK = doc.hasChild("fbos");
	if (!isOK) return;

	//
	if (isOK) {
		XmlTree mixXml = doc.getChild("fbos");
		fromXml(mixXml);
	}
}
void SDASession::fromXml(const XmlTree &xml) {

	// find fbo childs in xml
	if (xml.hasChild("fbo")) {
		CI_LOG_V("SDASession got fbo childs");
		for (XmlTree::ConstIter fboChild = xml.begin("fbo"); fboChild != xml.end(); ++fboChild) {
			CI_LOG_V("SDASession create fbo ");
			createShaderFbo(fboChild->getAttributeValue<string>("shadername", ""), 0);
		}
	}
}
// control values
void SDASession::toggleValue(unsigned int aCtrl) {
	mSDAWebsocket->toggleValue(aCtrl);
}
void SDASession::toggleAuto(unsigned int aCtrl) {
	mSDAWebsocket->toggleAuto(aCtrl);
}
void SDASession::toggleTempo(unsigned int aCtrl) {
	mSDAWebsocket->toggleTempo(aCtrl);
}
void SDASession::resetAutoAnimation(unsigned int aIndex) {
	mSDAWebsocket->resetAutoAnimation(aIndex);
}
float SDASession::getMinUniformValueByIndex(unsigned int aIndex) {
	return mSDAAnimation->getMinUniformValueByIndex(aIndex);
}
float SDASession::getMaxUniformValueByIndex(unsigned int aIndex) {
	return mSDAAnimation->getMaxUniformValueByIndex(aIndex);
}
void SDASession::updateMixUniforms() {
	//vec4 mouse = mSDAAnimation->getVec4UniformValueByName("iMouse");

	mGlslMix->uniform("iBlendmode", mSDASettings->iBlendmode);
	mGlslMix->uniform("iTime", mSDAAnimation->getFloatUniformValueByIndex(0));
	// was vec3(mSDASettings->mFboWidth, mSDASettings->mFboHeight, 1.0)):
	mGlslMix->uniform("iResolution", vec3(mSDAAnimation->getFloatUniformValueByName("iResolutionX"), mSDAAnimation->getFloatUniformValueByName("iResolutionY"), 1.0));
	//mGlslMix->uniform("iChannelResolution", mSDASettings->iChannelResolution, 4);
	// 20180318 mGlslMix->uniform("iMouse", mSDAAnimation->getVec4UniformValueByName("iMouse"));
	mGlslMix->uniform("iMouse", vec3(mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IMOUSEX), mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IMOUSEY), mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IMOUSEZ)));
	mGlslMix->uniform("iDate", mSDAAnimation->getVec4UniformValueByName("iDate"));
	mGlslMix->uniform("iWeight0", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT0));	// weight of channel 0
	mGlslMix->uniform("iWeight1", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT1));	// weight of channel 1
	mGlslMix->uniform("iWeight2", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT2));	// weight of channel 2
	mGlslMix->uniform("iWeight3", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT3)); // texture
	mGlslMix->uniform("iWeight4", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT4)); // texture
	mGlslMix->uniform("iChannel0", 0); // fbo shader 
	mGlslMix->uniform("iChannel1", 1); // fbo shader
	mGlslMix->uniform("iChannel2", 2); // texture 1
	mGlslMix->uniform("iChannel3", 3); // texture 2
	mGlslMix->uniform("iChannel4", 4); // texture 3

	mGlslMix->uniform("iRatio", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IRATIO));//check if needed: +1;
	mGlslMix->uniform("iRenderXY", mSDASettings->mRenderXY);
	mGlslMix->uniform("iZoom", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IZOOM));
	mGlslMix->uniform("iAlpha", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IFA) * mSDASettings->iAlpha);
	mGlslMix->uniform("iChromatic", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->ICHROMATIC));
	mGlslMix->uniform("iRotationSpeed", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IROTATIONSPEED));
	mGlslMix->uniform("iCrossfade", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IXFADE));
	mGlslMix->uniform("iPixelate", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IPIXELATE));
	mGlslMix->uniform("iExposure", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IEXPOSURE));
	mGlslMix->uniform("iToggle", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->ITOGGLE));
	mGlslMix->uniform("iGreyScale", (int)mSDASettings->iGreyScale);
	mGlslMix->uniform("iBackgroundColor", mSDAAnimation->getVec3UniformValueByName("iBackgroundColor"));
	mGlslMix->uniform("iVignette", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IVIGN));
	mGlslMix->uniform("iInvert", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IINVERT));
	mGlslMix->uniform("iTempoTime", mSDAAnimation->getFloatUniformValueByName("iTempoTime"));
	mGlslMix->uniform("iGlitch", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IGLITCH));
	mGlslMix->uniform("iTrixels", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->ITRIXELS));
	mGlslMix->uniform("iRedMultiplier", mSDAAnimation->getFloatUniformValueByName("iRedMultiplier"));
	mGlslMix->uniform("iGreenMultiplier", mSDAAnimation->getFloatUniformValueByName("iGreenMultiplier"));
	mGlslMix->uniform("iBlueMultiplier", mSDAAnimation->getFloatUniformValueByName("iBlueMultiplier"));
	mGlslMix->uniform("iFlipH", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IFLIPH));
	mGlslMix->uniform("iFlipV", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IFLIPV));
	mGlslMix->uniform("iParam1", mSDASettings->iParam1);
	mGlslMix->uniform("iParam2", mSDASettings->iParam2);
	mGlslMix->uniform("iXorY", mSDASettings->iXorY);
	mGlslMix->uniform("iBadTv", mSDAAnimation->getFloatUniformValueByName("iBadTv"));
	mGlslMix->uniform("iFps", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IFPS));
	mGlslMix->uniform("iContour", mSDAAnimation->getFloatUniformValueByName("iContour"));
	mGlslMix->uniform("iSobel", mSDAAnimation->getFloatUniformValueByName("iSobel"));

}
void SDASession::updateBlendUniforms() {
	mCurrentBlend = getElapsedFrames() % mSDAAnimation->getBlendModesCount();
	mGlslBlend->uniform("iBlendmode", mCurrentBlend);
	mGlslBlend->uniform("iTime", mSDAAnimation->getFloatUniformValueByIndex(0));
	mGlslBlend->uniform("iResolution", vec3(mSDASettings->mPreviewFboWidth, mSDASettings->mPreviewFboHeight, 1.0));
	//mGlslBlend->uniform("iChannelResolution", mSDASettings->iChannelResolution, 4);
	// 20180318 mGlslBlend->uniform("iMouse", mSDAAnimation->getVec4UniformValueByName("iMouse"));
	mGlslBlend->uniform("iMouse", vec3(mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IMOUSEX), mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IMOUSEY), mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IMOUSEZ)));
	mGlslBlend->uniform("iDate", mSDAAnimation->getVec4UniformValueByName("iDate"));
	mGlslBlend->uniform("iWeight0", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT0));	// weight of channel 0
	mGlslBlend->uniform("iWeight1", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT1));	// weight of channel 1
	mGlslBlend->uniform("iWeight2", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT2));	// weight of channel 2
	mGlslBlend->uniform("iWeight3", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT3)); // texture
	mGlslBlend->uniform("iWeight4", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT4)); // texture
	mGlslBlend->uniform("iChannel0", 0); // fbo shader 
	mGlslBlend->uniform("iChannel1", 1); // fbo shader
	mGlslBlend->uniform("iChannel2", 2); // texture 1
	mGlslBlend->uniform("iChannel3", 3); // texture 2
	mGlslBlend->uniform("iChannel4", 4); // texture 3
	mGlslBlend->uniform("iAudio0", 0);
	mGlslBlend->uniform("iFreq0", mSDAAnimation->getFloatUniformValueByName("iFreq0"));
	mGlslBlend->uniform("iFreq1", mSDAAnimation->getFloatUniformValueByName("iFreq1"));
	mGlslBlend->uniform("iFreq2", mSDAAnimation->getFloatUniformValueByName("iFreq2"));
	mGlslBlend->uniform("iFreq3", mSDAAnimation->getFloatUniformValueByName("iFreq3"));
	mGlslBlend->uniform("iChannelTime", mSDASettings->iChannelTime, 4);
	mGlslBlend->uniform("iColor", vec3(mSDAAnimation->getFloatUniformValueByIndex(1), mSDAAnimation->getFloatUniformValueByIndex(2), mSDAAnimation->getFloatUniformValueByIndex(3)));
	mGlslBlend->uniform("iBackgroundColor", mSDAAnimation->getVec3UniformValueByName("iBackgroundColor"));
	mGlslBlend->uniform("iSteps", (int)mSDAAnimation->getFloatUniformValueByIndex(10));
	mGlslBlend->uniform("iRatio", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IRATIO));
	mGlslBlend->uniform("width", 1);
	mGlslBlend->uniform("height", 1);
	mGlslBlend->uniform("iRenderXY", mSDASettings->mRenderXY);
	mGlslBlend->uniform("iZoom", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IZOOM));
	mGlslBlend->uniform("iAlpha", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IFA) * mSDASettings->iAlpha);
	mGlslBlend->uniform("iChromatic", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->ICHROMATIC));
	mGlslBlend->uniform("iRotationSpeed", mSDAAnimation->getFloatUniformValueByIndex(9));
	mGlslBlend->uniform("iCrossfade", 0.5f);// blendmode only work if different than 0 or 1
	mGlslBlend->uniform("iPixelate", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IPIXELATE));
	mGlslBlend->uniform("iExposure", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IEXPOSURE));
	mGlslBlend->uniform("iDeltaTime", mSDAAnimation->iDeltaTime);
	mGlslBlend->uniform("iFade", (int)mSDASettings->iFade);
	mGlslBlend->uniform("iToggle", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->ITOGGLE));
	mGlslBlend->uniform("iGreyScale", (int)mSDASettings->iGreyScale);
	mGlslBlend->uniform("iTransition", mSDASettings->iTransition);
	mGlslBlend->uniform("iAnim", mSDASettings->iAnim.value());
	mGlslBlend->uniform("iRepeat", (int)mSDASettings->iRepeat);
	mGlslBlend->uniform("iVignette", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IVIGN));
	mGlslBlend->uniform("iInvert", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IINVERT));
	mGlslBlend->uniform("iDebug", (int)mSDASettings->iDebug);
	mGlslBlend->uniform("iShowFps", (int)mSDASettings->iShowFps);
	mGlslBlend->uniform("iFps", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IFPS));
	mGlslBlend->uniform("iTempoTime", mSDAAnimation->getFloatUniformValueByName("iTempoTime"));
	mGlslBlend->uniform("iGlitch", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IGLITCH));
	mGlslBlend->uniform("iTrixels", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->ITRIXELS));
	//mGlslBlend->uniform("iPhase", mSDASettings->iPhase);
	mGlslBlend->uniform("iSeed", mSDASettings->iSeed);
	mGlslBlend->uniform("iRedMultiplier", mSDAAnimation->getFloatUniformValueByName("iRedMultiplier"));
	mGlslBlend->uniform("iGreenMultiplier", mSDAAnimation->getFloatUniformValueByName("iGreenMultiplier"));
	mGlslBlend->uniform("iBlueMultiplier", mSDAAnimation->getFloatUniformValueByName("iBlueMultiplier"));
	mGlslBlend->uniform("iFlipH", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IFLIPH));
	mGlslBlend->uniform("iFlipV", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IFLIPV));
	mGlslBlend->uniform("iParam1", mSDASettings->iParam1);
	mGlslBlend->uniform("iParam2", mSDASettings->iParam2);
	mGlslBlend->uniform("iXorY", mSDASettings->iXorY);
	mGlslBlend->uniform("iBadTv", mSDAAnimation->getFloatUniformValueByName("iBadTv"));
	mGlslBlend->uniform("iContour", mSDAAnimation->getFloatUniformValueByName("iContour"));
	mGlslBlend->uniform("iSobel", mSDAAnimation->getFloatUniformValueByName("iSobel"));

}
void SDASession::update(unsigned int aClassIndex) {

	if (aClassIndex == 0) {
		if (mSDAWebsocket->hasReceivedStream()) { //&& (getElapsedFrames() % 100 == 0)) {
			updateStream(mSDAWebsocket->getBase64Image());
		}
		if (mSDAWebsocket->hasReceivedShader()) {
			if (mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IXFADE) < 0.5) {
				setFragmentShaderString(2, mSDAWebsocket->getReceivedShader());
			}
			else {
				setFragmentShaderString(1, mSDAWebsocket->getReceivedShader());
			}
			// TODO timeline().apply(&mWarps[aWarpIndex]->ABCrossfade, 0.0f, 2.0f); };
		}
		/* TODO: CHECK index if (mSDASettings->iGreyScale)
		{
			mSDAWebsocket->changeFloatValue(1, mSDAAnimation->getFloatUniformValueByIndex(3));
			mSDAWebsocket->changeFloatValue(2, mSDAAnimation->getFloatUniformValueByIndex(3));
			mSDAWebsocket->changeFloatValue(5, mSDAAnimation->getFloatUniformValueByIndex(7));
			mSDAWebsocket->changeFloatValue(6, mSDAAnimation->getFloatUniformValueByIndex(7));
		}*/

		// fps calculated in main app
		mSDASettings->sFps = toString(floor(getFloatUniformValueByIndex(mSDASettings->IFPS)));
		mSDAAnimation->update();
	}
	else {
		// aClassIndex == 1 (audio analysis only)
		updateAudio();
	}
	// all cases
	mSDAWebsocket->update();
	if (mFreqWSSend) {
		mSDAWebsocket->changeFloatValue(mSDASettings->IFREQ0, getFreq(0), true);
		mSDAWebsocket->changeFloatValue(mSDASettings->IFREQ1, getFreq(1), true);
		mSDAWebsocket->changeFloatValue(mSDASettings->IFREQ2, getFreq(2), true);
		mSDAWebsocket->changeFloatValue(mSDASettings->IFREQ3, getFreq(3), true);
	}
	// check if xFade changed
	/*if (mSDASettings->xFadeChanged) {
		mSDASettings->xFadeChanged = false;
	}*/
	updateMixUniforms();
	renderMix();
	// blendmodes preview
	if (mSDAAnimation->renderBlend()) {
		updateBlendUniforms();
		renderBlend();
	}
}
bool SDASession::save()
{

	// save uniforms settings
	mSDAAnimation->save();
	// save in sessionPath
	JsonTree doc;

	JsonTree settings = JsonTree::makeArray("settings");
	settings.addChild(ci::JsonTree("bpm", mOriginalBpm));
	settings.addChild(ci::JsonTree("beatsperbar", mSDAAnimation->getIntUniformValueByName("iBeatsPerBar")));
	settings.addChild(ci::JsonTree("fadeindelay", mFadeInDelay));
	settings.addChild(ci::JsonTree("fadeoutdelay", mFadeOutDelay));
	settings.addChild(ci::JsonTree("endframe", mSDAAnimation->mEndFrame));
	doc.pushBack(settings);

	JsonTree assets = JsonTree::makeArray("assets");
	if (mWaveFileName.length() > 0) assets.addChild(ci::JsonTree("wavefile", mWaveFileName));
	assets.addChild(ci::JsonTree("waveplaybackdelay", mWavePlaybackDelay));
	if (mMovieFileName.length() > 0) assets.addChild(ci::JsonTree("moviefile", mMovieFileName));
	assets.addChild(ci::JsonTree("movieplaybackdelay", mMoviePlaybackDelay));
	if (mImageSequencePath.length() > 0) assets.addChild(ci::JsonTree("imagesequencepath", mImageSequencePath));
	if (mText.length() > 0) {
		assets.addChild(ci::JsonTree("text", mText));
		assets.addChild(ci::JsonTree("textplaybackdelay", mTextPlaybackDelay));
		assets.addChild(ci::JsonTree("textplaybackend", mTextPlaybackEnd));
	}
	doc.pushBack(assets);

	doc.write(writeFile(sessionPath), JsonTree::WriteOptions());

	return true;
}

void SDASession::restore()
{
	// save load settings
	load();

	// check to see if json file exists
	if (!fs::exists(sessionPath)) {
		return;
	}

	try {
		JsonTree doc(loadFile(sessionPath));
		if (doc.hasChild("settings")) {
			JsonTree settings(doc.getChild("settings"));
			if (settings.hasChild("bpm")) {
				mOriginalBpm = settings.getValueForKey<float>("bpm", 166.0f);
				CI_LOG_W("getBpm" + toString(mSDAAnimation->getBpm()) + " mOriginalBpm " + toString(mOriginalBpm));
				mSDAAnimation->setBpm(mOriginalBpm);
				CI_LOG_W("getBpm" + toString(mSDAAnimation->getBpm()));
			};
			if (settings.hasChild("beatsperbar")) mSDAAnimation->setIntUniformValueByName("iBeatsPerBar", settings.getValueForKey<int>("beatsperbar"));
			if (mSDAAnimation->getIntUniformValueByName("iBeatsPerBar") < 1) mSDAAnimation->setIntUniformValueByName("iBeatsPerBar", 4);
			if (settings.hasChild("fadeindelay")) mFadeInDelay = settings.getValueForKey<int>("fadeindelay");
			if (settings.hasChild("fadeoutdelay")) mFadeOutDelay = settings.getValueForKey<int>("fadeoutdelay");
			if (settings.hasChild("endframe")) mSDAAnimation->mEndFrame = settings.getValueForKey<int>("endframe");
			CI_LOG_W("getBpm" + toString(mSDAAnimation->getBpm()) + " mTargetFps " + toString(mTargetFps));
			mTargetFps = mSDAAnimation->getBpm() / 60.0f * mFpb;
			CI_LOG_W("getBpm" + toString(mSDAAnimation->getBpm()) + " mTargetFps " + toString(mTargetFps));
		}

		if (doc.hasChild("assets")) {
			JsonTree assets(doc.getChild("assets"));
			if (assets.hasChild("wavefile")) mWaveFileName = assets.getValueForKey<string>("wavefile");
			if (assets.hasChild("waveplaybackdelay")) mWavePlaybackDelay = assets.getValueForKey<int>("waveplaybackdelay");
			if (assets.hasChild("moviefile")) mMovieFileName = assets.getValueForKey<string>("moviefile");
			if (assets.hasChild("movieplaybackdelay")) mMoviePlaybackDelay = assets.getValueForKey<int>("movieplaybackdelay");
			if (assets.hasChild("imagesequencepath")) mImageSequencePath = assets.getValueForKey<string>("imagesequencepath");
			if (assets.hasChild("text")) mText = assets.getValueForKey<string>("text");
			if (assets.hasChild("textplaybackdelay")) mTextPlaybackDelay = assets.getValueForKey<int>("textplaybackdelay");
			if (assets.hasChild("textplaybackend")) mTextPlaybackEnd = assets.getValueForKey<int>("textplaybackend");
		}

	}
	catch (const JsonTree::ExcJsonParserError& exc) {
		CI_LOG_W(exc.what());
	}
}

void SDASession::resetSomeParams() {
	// parameters not exposed in json file
	mFpb = 16;
	mSDAAnimation->setBpm(mOriginalBpm);
	mTargetFps = mOriginalBpm / 60.0f * mFpb;
}

void SDASession::reset()
{
	// parameters exposed in json file
	mOriginalBpm = 166;
	mWaveFileName = "";
	mWavePlaybackDelay = 10;
	mMovieFileName = "";
	mImageSequencePath = "";
	mMoviePlaybackDelay = 10;
	mFadeInDelay = 5;
	mFadeOutDelay = 1;
	mSDAAnimation->mEndFrame = 20000000;
	mText = "";
	mTextPlaybackDelay = 10;
	mTextPlaybackEnd = 2020000;

	resetSomeParams();
}
int SDASession::getWindowsResolution() {
	return mSDAUtils->getWindowsResolution();
}
void SDASession::blendRenderEnable(bool render) {
	mSDAAnimation->blendRenderEnable(render);
}

void SDASession::fileDrop(FileDropEvent event) {
	string ext = "";
	//string fileName = "";

	unsigned int index = (int)(event.getX() / (mSDASettings->uiLargePreviewW + mSDASettings->uiMargin));
	int y = (int)(event.getY());
	if (index < 2 || y < mSDASettings->uiYPosRow3 || y > mSDASettings->uiYPosRow3 + mSDASettings->uiPreviewH) index = 0;
	ci::fs::path mPath = event.getFile(event.getNumFiles() - 1);
	string absolutePath = mPath.string();
	// use the last of the dropped files
	int dotIndex = absolutePath.find_last_of(".");
	int slashIndex = absolutePath.find_last_of("\\");

	if (dotIndex != std::string::npos && dotIndex > slashIndex) {
		ext = absolutePath.substr(dotIndex + 1);
		//fileName = absolutePath.substr(slashIndex + 1, dotIndex - slashIndex - 1);


		if (ext == "wav" || ext == "mp3") {
			loadAudioFile(absolutePath);
		}
		else if (ext == "png" || ext == "jpg") {
			if (index < 1) index = 1;
			if (index > 3) index = 3;
			loadImageFile(absolutePath, index);
		}
		else if (ext == "glsl" || ext == "frag" || ext == "fs") {
			loadFragmentShader(absolutePath);
		}
		else if (ext == "xml") {
		}
		else if (ext == "mov") {
			loadMovie(absolutePath, index);
		}
		else if (ext == "txt") {
		}
		else if (ext == "") {
			// try loading image sequence from dir
			if (!loadImageSequence(absolutePath, index)) {
				// try to load a folder of shaders
				loadShaderFolder(absolutePath);
			}
		}
	}
}
#pragma region events
bool SDASession::handleMouseMove(MouseEvent &event)
{
	// 20180318 handled in SDAUIMouse mSDAAnimation->setVec4UniformValueByIndex(70, vec4(event.getX(), event.getY(), event.isLeftDown(), event.isRightDown()));
	// pass this mouse event to the warp editor first	
	bool handled = false;
	event.setHandled(handled);
	return event.isHandled();
}

bool SDASession::handleMouseDown(MouseEvent &event)
{
	bool handled = false;
	// 20180318 handled in SDAUIMouse mSDAAnimation->setVec4UniformValueByIndex(70, vec4(event.getX(), event.getY(), event.isLeftDown(), event.isRightDown()));
	event.setHandled(handled);
	return event.isHandled();
}

bool SDASession::handleMouseDrag(MouseEvent &event)
{
	bool handled = false;
	// 20180318 handled in SDAUIMouse mSDAAnimation->setVec4UniformValueByIndex(70, vec4(event.getX(), event.getY(), event.isLeftDown(), event.isRightDown()));
	event.setHandled(handled);
	return event.isHandled();
}

bool SDASession::handleMouseUp(MouseEvent &event)
{
	bool handled = false;
	// 20180318 handled in SDAUIMouse mSDAAnimation->setVec4UniformValueByIndex(70, vec4(event.getX(), event.getY(), event.isLeftDown(), event.isRightDown()));
	event.setHandled(handled);
	return event.isHandled();
}

bool SDASession::handleKeyDown(KeyEvent &event)
{
	bool handled = true;
	float newValue;
#if defined( CINDER_COCOA )
	bool isModDown = event.isMetaDown();
#else // windows
	bool isModDown = event.isControlDown();
#endif
	bool isShiftDown = event.isShiftDown();
	bool isAltDown = event.isAltDown();
	CI_LOG_V("session keydown: " + toString(event.getCode()) + " ctrl: " + toString(isModDown) + " shift: " + toString(isShiftDown) + " alt: " + toString(isAltDown));

	// pass this event to Mix handler
	if (!mSDAAnimation->handleKeyDown(event)) {
		switch (event.getCode()) {

		case KeyEvent::KEY_SPACE:
			//mSDATextures->playMovie();
			//mSDAAnimation->currentScene++;
			//if (mMovie) { if (mMovie->isPlaying()) mMovie->stop(); else mMovie->play(); }
			break;
		case KeyEvent::KEY_0:
			break;
		case KeyEvent::KEY_l:
			// live params TODO mSDAAnimation->load();
			//mLoopVideo = !mLoopVideo;
			//if (mMovie) mMovie->setLoop(mLoopVideo);
			break;
		case KeyEvent::KEY_x:
			// trixels
			mSDAWebsocket->changeFloatValue(mSDASettings->ITRIXELS, mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->ITRIXELS) + 0.05f);
			break;
		case KeyEvent::KEY_r:
			if (isAltDown) {
				mSDAWebsocket->changeFloatValue(mSDASettings->IBR, mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IBR), false, true, isShiftDown, isModDown);
			}
			else {
				mSDAWebsocket->changeFloatValue(mSDASettings->IFR, mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IFR), false, true, isShiftDown, isModDown);
			}
			break;
		case KeyEvent::KEY_g:
			if (isAltDown) {
				mSDAWebsocket->changeFloatValue(mSDASettings->IBG, mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IBG), false, true, isShiftDown, isModDown);
			}
			else {
				mSDAWebsocket->changeFloatValue(mSDASettings->IFG, mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IFG), false, true, isShiftDown, isModDown);
			}
			break;
		case KeyEvent::KEY_b:
			if (isAltDown) {
				mSDAWebsocket->changeFloatValue(mSDASettings->IBB, mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IBB), false, true, isShiftDown, isModDown);
			}
			else {
				mSDAWebsocket->changeFloatValue(mSDASettings->IFB, mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IFB), false, true, isShiftDown, isModDown);
			}
			break;
		case KeyEvent::KEY_a:
			mSDAWebsocket->changeFloatValue(mSDASettings->IFA, mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IFA), false, true, isShiftDown, isModDown);
			break;
		case KeyEvent::KEY_c:
			// chromatic
			// TODO find why can't put value >0.9 or 0.85!
			newValue = mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->ICHROMATIC) + 0.05f;
			if (newValue > 1.0f) newValue = 0.0f;
			mSDAWebsocket->changeFloatValue(mSDASettings->ICHROMATIC, newValue);
			break;
		case KeyEvent::KEY_p:
			// pixelate
			mSDAWebsocket->changeFloatValue(mSDASettings->IPIXELATE, mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IPIXELATE) + 0.05f);
			break;
		case KeyEvent::KEY_y:
			// glitch
			mSDAWebsocket->changeBoolValue(mSDASettings->IGLITCH, true);
			break;
		case KeyEvent::KEY_i:
			// invert
			mSDAWebsocket->changeBoolValue(mSDASettings->IINVERT, true);
			break;
		case KeyEvent::KEY_o:
			// toggle
			mSDAWebsocket->changeBoolValue(mSDASettings->ITOGGLE, true);
			break;
		case KeyEvent::KEY_z:
			// zoom
			mSDAWebsocket->changeFloatValue(mSDASettings->IZOOM, mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IZOOM) - 0.05f);
			break;
			/* removed temp for Sky Project case KeyEvent::KEY_LEFT:
				//mSDATextures->rewindMovie();
				if (mSDAAnimation->getFloatUniformValueByIndex(21) > 0.1f) mSDAWebsocket->changeFloatValue(21, mSDAAnimation->getFloatUniformValueByIndex(21) - 0.1f);
				break;
			case KeyEvent::KEY_RIGHT:
				//mSDATextures->fastforwardMovie();
				if (mSDAAnimation->getFloatUniformValueByIndex(21) < 1.0f) mSDAWebsocket->changeFloatValue(21, mSDAAnimation->getFloatUniformValueByIndex(21) + 0.1f);
				break;*/
		case KeyEvent::KEY_PAGEDOWN:
			// crossfade right
			if (mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IXFADE) < 1.0f) mSDAWebsocket->changeFloatValue(mSDASettings->IXFADE, mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IXFADE) + 0.1f);
			break;
		case KeyEvent::KEY_PAGEUP:
			// crossfade left
			if (mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IXFADE) > 0.0f) mSDAWebsocket->changeFloatValue(mSDASettings->IXFADE, mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IXFADE) - 0.1f);
			break;
		default:	
			CI_LOG_V("session keydown: " + toString(event.getCode()));			
			handled = false;
			break;
		}
	}
	CI_LOG_V((handled ? "session keydown handled " : "session keydown not handled "));
	event.setHandled(handled);
	return event.isHandled();
}
bool SDASession::handleKeyUp(KeyEvent &event) {
	bool handled = true;


	if (!mSDAAnimation->handleKeyUp(event)) {
		// Animation did not handle the key, so handle it here
		switch (event.getCode()) {
		case KeyEvent::KEY_y:
			// glitch
			mSDAWebsocket->changeBoolValue(mSDASettings->IGLITCH, false);
			break;
		case KeyEvent::KEY_t:
			// trixels
			mSDAWebsocket->changeFloatValue(mSDASettings->ITRIXELS, 0.0f);
			break;
		case KeyEvent::KEY_i:
			// invert
			mSDAWebsocket->changeBoolValue(mSDASettings->IINVERT, false);
			break;
		case KeyEvent::KEY_c:
			// chromatic
			mSDAWebsocket->changeFloatValue(mSDASettings->ICHROMATIC, 0.0f);
			break;
		case KeyEvent::KEY_p:
			// pixelate
			mSDAWebsocket->changeFloatValue(mSDASettings->IPIXELATE, 1.0f);
			break;
		case KeyEvent::KEY_o:
			// toggle
			mSDAWebsocket->changeBoolValue(mSDASettings->ITOGGLE, false);
			break;
		case KeyEvent::KEY_z:
			// zoom
			mSDAWebsocket->changeFloatValue(mSDASettings->IZOOM, 1.0f);
			break;
		default:
			CI_LOG_V("session keyup: " + toString(event.getCode()));
			handled = false;
			break;
		}
	}
	CI_LOG_V((handled ? "session keyup handled " : "session keyup not handled "));
	event.setHandled(handled);
	return event.isHandled();
}
#pragma endregion events
// fbos
#pragma region fbos



int SDASession::loadFragmentShader(string aFilePath) {
	int rtn = -1;
	CI_LOG_V("loadFragmentShader " + aFilePath);
	createShaderFbo(aFilePath);

	return rtn;
}

void SDASession::sendFragmentShader(unsigned int aShaderIndex) {
	mSDAWebsocket->changeFragmentShader(getFragmentString(aShaderIndex));
}

void SDASession::setFboAIndex(unsigned int aIndex, unsigned int aFboIndex) {
	if (aFboIndex < mFboList.size()) {
		mAFboIndex = aFboIndex;
	}
	else {
		mAFboIndex = mFboList.size() - 1;
	}
	/*mSDAMix->setWarpAFboIndex(aIndex, aFboIndex);
	mSDARouter->setWarpAFboIndex(aIndex, aFboIndex);
	mSDAWebsocket->changeWarpFboIndex(aIndex, aFboIndex, 0);*/
}
void SDASession::setFboBIndex(unsigned int aIndex, unsigned int aFboIndex) {
	if (aFboIndex < mFboList.size()) {
		mBFboIndex = aFboIndex;
	}
	else {
		mBFboIndex = mFboList.size() - 1;
	}
	/*mSDAMix->setWarpBFboIndex(aIndex, aFboIndex);
	mSDARouter->setWarpBFboIndex(aIndex, aFboIndex);
	mSDAWebsocket->changeWarpFboIndex(aIndex, aFboIndex, 1);*/
}
#pragma endregion fbos
// shaders
#pragma region shaders
bool SDASession::loadShaderFolder(string aFolder) {
	string ext = "";
	fs::path p(aFolder);
	for (fs::directory_iterator it(p); it != fs::directory_iterator(); ++it)
	{
		if (fs::is_regular_file(*it))
		{
			string fileName = it->path().filename().string();
			int dotIndex = fileName.find_last_of(".");

			if (dotIndex != std::string::npos)
			{
				ext = fileName.substr(dotIndex + 1);
				if (ext == "glsl")
				{
					loadFragmentShader(aFolder + "/" + fileName);
				}
			}
		}
	}
	return true;
}

#pragma endregion shaders

// websockets
#pragma region websockets

void SDASession::wsConnect() {
	mSDAWebsocket->wsConnect();
}
void SDASession::wsPing() {
	mSDAWebsocket->wsPing();
}
void SDASession::wsWrite(string msg)
{
	mSDAWebsocket->wsWrite(msg);
}
#pragma endregion websockets

// mix
#pragma region mix
void SDASession::setFboFragmentShaderIndex(unsigned int aFboIndex, unsigned int aFboShaderIndex) {
	CI_LOG_V("setFboFragmentShaderIndex, before, fboIndex: " + toString(aFboIndex) + " shaderIndex " + toString(aFboShaderIndex));
	if (aFboIndex > mFboList.size() - 1) aFboIndex = mFboList.size() - 1;
	if (aFboShaderIndex > mShaderList.size() - 1) aFboShaderIndex = mShaderList.size() - 1;
	CI_LOG_V("setFboFragmentShaderIndex, after, fboIndex: " + toString(aFboIndex) + " shaderIndex " + toString(aFboShaderIndex));
	mFboList[aFboIndex]->setFragmentShader(aFboShaderIndex, mShaderList[aFboShaderIndex]->getFragmentString(), mShaderList[aFboShaderIndex]->getName());
	// route message
	// LOOP! mSDAWebsocket->changeFragmentShader(mShaderList[aFboShaderIndex]->getFragmentString());
}

unsigned int SDASession::getFboFragmentShaderIndex(unsigned int aFboIndex) {
	unsigned int rtn = mFboList[aFboIndex]->getShaderIndex();
	//CI_LOG_V("getFboFragmentShaderIndex, fboIndex: " + toString(aFboIndex)+" shaderIndex: " + toString(rtn));
	if (rtn > mShaderList.size() - 1) rtn = mShaderList.size() - 1;
	return rtn;
}
string SDASession::getShaderName(unsigned int aShaderIndex) {
	if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
	return mShaderList[aShaderIndex]->getName();
}
ci::gl::TextureRef SDASession::getShaderThumb(unsigned int aShaderIndex) {
	unsigned int found = 0;
	for (int i = 0; i < mFboList.size(); i++)
	{
		if (mFboList[i]->getShaderIndex() == aShaderIndex) found = i;
	}
	return getFboRenderedTexture(found);
}
void SDASession::updateStream(string * aStringPtr) {
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
string SDASession::getFragmentShaderString(unsigned int aShaderIndex) {
	if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
	return mShaderList[aShaderIndex]->getFragmentString();
}
// shaders
void SDASession::updateShaderThumbFile(unsigned int aShaderIndex) {
	for (int i = 0; i < mFboList.size(); i++)
	{
		if (mFboList[i]->getShaderIndex() == aShaderIndex) mFboList[i]->updateThumbFile();
	}
}
void SDASession::removeShader(unsigned int aShaderIndex) {
	if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
	mShaderList[aShaderIndex]->removeShader();
}
void SDASession::setFragmentShaderString(unsigned int aShaderIndex, string aFragmentShaderString, string aName) {
	if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
	mShaderList[aShaderIndex]->setFragmentString(aFragmentShaderString, aName);
	// if live coding shader compiles and is used by a fbo reload it
	for (int i = 0; i < mFboList.size(); i++)
	{
		if (mFboList[i]->getShaderIndex() == aShaderIndex) setFboFragmentShaderIndex(i, aShaderIndex);
	}
}
unsigned int SDASession::createShaderFboFromString(string aFragmentShaderString, string aShaderFilename) {
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
		fboXml.setAttribute("width", "1280");
		fboXml.setAttribute("height", "720");
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

/*string SDASession::getVertexShaderString(unsigned int aShaderIndex) {
	if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
	return mShaderList[aShaderIndex]->getVertexString();
}*/


unsigned int SDASession::createShaderFbo(string aShaderFilename, unsigned int aInputTextureIndex) {
	// initialize rtn to 0 to force creation
	unsigned int rtn = 0;
	string fName = aShaderFilename;
	string ext = "";
	int dotIndex = fName.find_last_of(".");
	int slashIndex = fName.find_last_of("\\");
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
				dotIndex = fName.find_last_of(".");
				slashIndex = fName.find_last_of("\\");

				if (dotIndex != std::string::npos && dotIndex > slashIndex) {
					ext = fName.substr(dotIndex + 1);
				}
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
void SDASession::setFboInputTexture(unsigned int aFboIndex, unsigned int aInputTextureIndex) {
	if (aFboIndex > mFboList.size() - 1) aFboIndex = mFboList.size() - 1;
	if (aInputTextureIndex > mTextureList.size() - 1) aInputTextureIndex = mTextureList.size() - 1;
	mFboList[aFboIndex]->setInputTexture(mTextureList, aInputTextureIndex);
}
unsigned int SDASession::getFboInputTextureIndex(unsigned int aFboIndex) {
	if (aFboIndex > mFboList.size() - 1) aFboIndex = mFboList.size() - 1;
	return mFboList[aFboIndex]->getInputTextureIndex();
}
void SDASession::initShaderList() {

	if (mShaderList.size() == 0) {
		CI_LOG_V("SDASession::init mShaderList");
		createShaderFboFromString("void main(void){vec2 uv = gl_FragCoord.xy / iResolution.xy;uv = abs(2.0*(uv - 0.5));vec4 t1 = texture2D(iChannel0, vec2(uv[0], 0.1));vec4 t2 = texture2D(iChannel0, vec2(uv[1], 0.1));float fft = t1[0] * t2[0];gl_FragColor = vec4(sin(fft*3.141*2.5), sin(fft*3.141*2.0), sin(fft*3.141*1.0), 1.0);}", "fftMatrixProduct.glsl");
		createShaderFboFromString("void main(void){vec2 uv = fragCoord.xy / iResolution.xy;vec4 tex = texture(iChannel0, uv);fragColor = vec4(vec3( tex.r, tex.g, tex.b ),1.0);}", "0.glsl");
		createShaderFboFromString("void main(void){vec2 uv = fragCoord.xy / iResolution.xy;vec4 tex = texture(iChannel1, uv);fragColor = vec4(vec3( tex.r, tex.g, tex.b ),1.0);}", "1.glsl");
		createShaderFboFromString("void main(void) {vec2 uv = 2 * (gl_FragCoord.xy / iResolution.xy - vec2(0.5));float radius = length(uv);float angle = atan(uv.y, uv.x);float col = .0;col += 1.5*sin(iTime + 13.0 * angle + uv.y * 20);col += cos(.9 * uv.x * angle * 60.0 + radius * 5.0 - iTime * 2.);fragColor = (1.2 - radius) * vec4(vec3(col), 1.0);}", "hexler330.glsl");

		createShaderFboFromString("void main(void){vec2 uv = 2 * (fragCoord.xy / iResolution.xy - vec2(0.5));float specx = texture2D( iChannel0, vec2(0.25,5.0/100.0) ).x;float specy = texture2D( iChannel0, vec2(0.5,5.0/100.0) ).x;float specz = 1.0*texture2D( iChannel0, vec2(0.7,5.0/100.0) ).x;float r = length(uv); float p = atan(uv.y/uv.x); uv = abs(uv);float col = 0.0;float amp = (specx+specy+specz)/3.0;uv.y += sin(uv.y*3.0*specx-iTime/5.0*specy+r*10.);uv.x += cos((iTime/5.0)+specx*30.0*uv.x);col += abs(1.0/uv.y/30.0) * (specx+specz)*15.0;col += abs(1.0/uv.x/60.0) * specx*8. ; fragColor=vec4(vec3( col ),1.0);}", "Hexler2.glsl");
		createShaderFboFromString("void main(void){vec2 uv = 2 * (gl_FragCoord.xy / iResolution.xy - vec2(0.5));vec2 spec = 1.0*texture2D(iChannel0, vec2(0.25, 5.0 / 100.0)).xx;float col = 0.0;uv.x += sin(iTime * 6.0 + uv.y*1.5)*spec.y;col += abs(0.8 / uv.x) * spec.y;gl_FragColor = vec4(col, col, col, 1.0);}", "SoundVizVert.glsl");
		createShaderFboFromString("void main(void){vec2 uv = 2 * (gl_FragCoord.xy / iResolution.xy - vec2(0.5));vec2 spec = 1.0*texture2D(iChannel0, vec2(0.25, 5.0 / 100.0)).yy;float col = 0.0;uv.y += sin(iTime * 6.0 + uv.x*1.5)*spec.x;col += abs(0.8/uv.y) * spec.x;gl_FragColor = vec4(col, col, col, 1.0);}", "SoundVizHoriz.glsl");
		//createShaderFboFromString("#define f(a,b)sin(50.3*length(fragCoord.xy/iResolution.xy*4.-vec2(cos(a),sin(b))-3.)) \n void main(){float t=iTime;fragColor=vec4(f(t,t)*f(1.4*t,.7*t));}", "Hyper-lightweight2XOR", "Hyper-lightweight2XOR.glsl");
		createShaderFboFromString("void main(void) {float d = pow(dot(fragCoord.xy, iResolution.xy ), 0.52); d =  d * 0.5;float x = sin(6.0+0.1*d + iTime*-6.0) * 10.0;fragColor = vec4( x, x, x, 1 );}", "WallSide.glsl");

		createShaderFboFromString("void main(void){float d = distance(fragCoord.xy, iResolution.xy * vec2(0.5,0.5).xy);float x = sin(5.0+0.1*d + iTime*-4.0) * 5.0;x = clamp( x, 0.0, 1.0 );fragColor = vec4(x, x, x, 1.0);}", "Circular.glsl");
		createShaderFboFromString("void main(void){vec4 p = vec4(fragCoord.xy,0.,1.)/iResolution.y - vec4(.9,.5,0,0), c=p-p;float t=iTime,r=length(p.xy+=sin(t+sin(t*.8))*.4),a=atan(p.y,p.x);for (float i = 0.;i<60.;i++) c = c*.98 + (sin(i+vec4(5,3,2,1))*.5+.5)*smoothstep(.99, 1., sin(log(r+i*.05)-t-i+sin(a +=t*.01)));fragColor = c*r;}", "2TweetsChallenge.glsl");
		createShaderFboFromString("void main(void){vec2 p = -1.0+2.0*fragCoord.xy/iResolution.xy;float w = sin(iTime+6.5*sqrt(dot(p,p))*cos(p.x));float x = cos(int(iRatio*10.0)*atan(p.y,p.x) + 1.8*w);vec3 col = iColor*15.0;fragColor = vec4(col*x,1.0);}", "gunstonSmoke.glsl");
		createShaderFboFromString("void main(void) {vec2  px = 4.0*(-iResolution.xy + 2.0*fragCoord.xy)/iResolution.y;float id = 0.5 + 0.5*cos(iTime + sin(dot(floor(px+0.5),vec2(113.1,17.81)))*43758.545);vec3 co = 0.5 + 0.5*cos(iTime + 3.5*id + vec3(0.0,1.57,3.14) );vec2 pa = smoothstep( 0.0, 0.2, id*(0.5 + 0.5*cos(6.2831*px)) );fragColor = vec4( co*pa.x*pa.y, 1.0 );}", "ColorGrid.glsl");
		createShaderFboFromString("void main(void){vec2 uv = gl_FragCoord.xy / iResolution.xy;fragColor = texture(iChannel0, uv);}", "tex0");
		createShaderFboFromString("void main(void){vec2 uv = gl_FragCoord.xy / iResolution.xy;fragColor = texture(iChannel0, uv);}", "tex1");
	}
}
bool SDASession::initTextureList() {
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
						xml.setAttribute("width", 1280);
						xml.setAttribute("height", 720);
						t->fromXml(xml);
						mTextureList.push_back(t);
					}
				}
			}
		}
	}
	return isFirstLaunch;
}
void SDASession::fboFlipV(unsigned int aFboIndex) {
	if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
	mFboList[aFboIndex]->flipV();
}
bool SDASession::isFboFlipV(unsigned int aFboIndex) {
	if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
	return mFboList[aFboIndex]->isFlipV();
}

#pragma endregion mix

#pragma region textures
ci::gl::TextureRef SDASession::getInputTexture(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->getTexture();
}
/*ci::gl::TextureRef SDASession::getNextInputTexture(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	if (mTextureList[aTextureIndex]->getType() == mTextureList[aTextureIndex]->SEQUENCE) {
		return mTextureList[aTextureIndex]->getNextTexture();
	}
	else {
		return mTextureList[aTextureIndex]->getTexture();
	}
	
}*/
string SDASession::getInputTextureName(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->getName();
}
unsigned int SDASession::getInputTexturesCount() {
	return mTextureList.size();
}
unsigned int SDASession::getInputTextureOriginalWidth(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->getOriginalWidth();
}
unsigned int SDASession::getInputTextureOriginalHeight(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->getOriginalHeight();
}
int SDASession::getInputTextureXLeft(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->getXLeft();
}
void SDASession::setInputTextureXLeft(unsigned int aTextureIndex, int aXLeft) {
	mTextureList[aTextureIndex]->setXLeft(aXLeft);
}
int SDASession::getInputTextureYTop(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->getYTop();
}
void SDASession::setInputTextureYTop(unsigned int aTextureIndex, int aYTop) {
	mTextureList[aTextureIndex]->setYTop(aYTop);
}
int SDASession::getInputTextureXRight(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->getXRight();
}
void SDASession::setInputTextureXRight(unsigned int aTextureIndex, int aXRight) {
	mTextureList[aTextureIndex]->setXRight(aXRight);
}
int SDASession::getInputTextureYBottom(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->getYBottom();
}
void SDASession::setInputTextureYBottom(unsigned int aTextureIndex, int aYBottom) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	mTextureList[aTextureIndex]->setYBottom(aYBottom);
}
bool SDASession::isFlipVInputTexture(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->isFlipV();
}
void SDASession::inputTextureFlipV(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	mTextureList[aTextureIndex]->flipV();
}
bool SDASession::isFlipHInputTexture(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->isFlipH();
}
void SDASession::inputTextureFlipH(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	mTextureList[aTextureIndex]->flipH();
}

bool SDASession::getInputTextureLockBounds(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->getLockBounds();
}
void SDASession::toggleInputTextureLockBounds(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	mTextureList[aTextureIndex]->toggleLockBounds();
}
void SDASession::togglePlayPause(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	mTextureList[aTextureIndex]->togglePlayPause();
}
bool SDASession::loadImageSequence(string aFolder, unsigned int aTextureIndex) {
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
void SDASession::loadMovie(string aFile, unsigned int aTextureIndex) {

}
void SDASession::loadImageFile(string aFile, unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	CI_LOG_V("loadImageFile " + aFile + " at textureIndex " + toString(aTextureIndex));
	mTextureList[aTextureIndex]->loadFromFullPath(aFile);
}
void SDASession::loadAudioFile(string aFile) {
	mTextureList[0]->loadFromFullPath(aFile);
}
bool SDASession::isMovie(unsigned int aTextureIndex) {
	return false;
}

// sequence
bool SDASession::isSequence(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return (mTextureList[aTextureIndex]->getType() == mTextureList[aTextureIndex]->SEQUENCE);
}
bool SDASession::isLoadingFromDisk(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return (mTextureList[aTextureIndex]->isLoadingFromDisk());
}
void SDASession::toggleLoadingFromDisk(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	mTextureList[aTextureIndex]->toggleLoadingFromDisk();
}
void SDASession::syncToBeat(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	mTextureList[aTextureIndex]->syncToBeat();
}
void SDASession::reverse(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	mTextureList[aTextureIndex]->reverse();
}
float SDASession::getSpeed(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->getSpeed();
}
void SDASession::setSpeed(unsigned int aTextureIndex, float aSpeed) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	mTextureList[aTextureIndex]->setSpeed(aSpeed);
}
int SDASession::getPosition(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->getPosition();
}
void SDASession::setPlayheadPosition(unsigned int aTextureIndex, int aPosition) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	mTextureList[aTextureIndex]->setPlayheadPosition(aPosition);
}
int SDASession::getMaxFrame(unsigned int aTextureIndex) {
	if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
	return mTextureList[aTextureIndex]->getMaxFrame();
}
#pragma endregion textures
void SDASession::load()
{
	CI_LOG_V("SDAMix load: ");
	CI_LOG_V("mMixFbos.size() < mWarps.size(), we create a new mixFbo");
	mMixFbos[0].fbo = gl::Fbo::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight, fboFmt);
	mMixFbos[0].texture = gl::Texture2d::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight);
	mMixFbos[0].name = "new";
}

// Render the scene into the FBO
ci::gl::Texture2dRef SDASession::getRenderTexture()
{
	gl::ScopedFramebuffer fbScp(mRenderFbo);
	gl::clear(Color::black());
	// setup the viewport to match the dimensions of the FBO
	gl::ScopedViewport scpVp(ivec2(0), mRenderFbo->getSize());


	mRenderedTexture = mRenderFbo->getColorTexture();
	return mRenderedTexture;
}

ci::gl::TextureRef SDASession::getMixTexture(unsigned int aMixFboIndex) {
	if (aMixFboIndex > mMixFbos.size() - 1) aMixFboIndex = 0;
	if (!mMixFbos[aMixFboIndex].texture) {
		// should never happen 
		mMixFbos[aMixFboIndex].texture = gl::Texture2d::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight);
	}
	if (!mMixFbos[aMixFboIndex].fbo) {
		// should never happen
		mMixFbos[aMixFboIndex].fbo = gl::Fbo::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight, fboFmt);
	}

	return mMixFbos[aMixFboIndex].texture;
}

ci::gl::TextureRef SDASession::getFboTexture(unsigned int aFboIndex) {
	if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
	return mFboList[aFboIndex]->getFboTexture();
}
ci::gl::TextureRef SDASession::getFboRenderedTexture(unsigned int aFboIndex) {
	if (aFboIndex > mFboList.size() - 1) aFboIndex = 0;
	return mFboList[aFboIndex]->getRenderedTexture();
}

void SDASession::renderBlend()
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

void SDASession::renderMix() {
	if (mFboList.size() > 0) {
		if (!mMixFbos[0].fbo) mMixFbos[0].fbo = gl::Fbo::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight, fboFmt);
		gl::ScopedFramebuffer scopedFbo(mMixFbos[0].fbo);
		gl::clear(Color::black());
		// render A and B fbos 
		mFboList[mAFboIndex]->getFboTexture();
		mFboList[mBFboIndex]->getFboTexture();
		// texture binding must be before ScopedGlslProg
		mFboList[mAFboIndex]->getRenderedTexture()->bind(0);
		mFboList[mBFboIndex]->getRenderedTexture()->bind(1);
		gl::ScopedGlslProg glslScope(mGlslMix);
		mGlslMix->uniform("iCrossfade", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IXFADE));

		gl::drawSolidRect(Rectf(0, 0, mMixFbos[0].fbo->getWidth(), mMixFbos[0].fbo->getHeight()));

		// save to a texture
		mMixFbos[0].texture = mMixFbos[0].fbo->getColorTexture();
	}
}

string SDASession::getMixFboName(unsigned int aMixFboIndex) {
	if (aMixFboIndex > mMixFbos.size() - 1) aMixFboIndex = mMixFbos.size() - 1;
	mMixFbos[aMixFboIndex].name = mFboList[0]->getName() + "/" + mFboList[1]->getName();
	return mMixFbos[aMixFboIndex].name;
}

