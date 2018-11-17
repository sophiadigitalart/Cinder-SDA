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
	mSDAAnimation->tapTempo();

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

	mGlslMix = gl::GlslProg::create(mSDASettings->getDefaultVextexShaderString(), mSDASettings->getMixFragmentShaderString());
	// 20161209 problem on Mac mGlslMix->setLabel("mixfbo");
	mGlslBlend = gl::GlslProg::create(mSDASettings->getDefaultVextexShaderString(), mSDASettings->getMixFragmentShaderString());
	// 20161209 problem on Mac mGlslBlend->setLabel("blend mixfbo");

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

void SDASession::update(unsigned int aClassIndex) {

	if (aClassIndex == 0) {
		if (mSDAWebsocket->hasReceivedStream() && (getElapsedFrames() % 100 == 0)) {
			updateStream(mSDAWebsocket->getBase64Image());
		}
		if (mSDAWebsocket->hasReceivedShader()) {
			if (mSDASettings->xFade < 0.5) {
				setFragmentShaderString(2, mSDAWebsocket->getReceivedShader());
			}
			else {
				setFragmentShaderString(1, mSDAWebsocket->getReceivedShader());
			}
		}
		if (mSDASettings->iGreyScale)
		{
			mSDAWebsocket->changeFloatValue(1, mSDAAnimation->getFloatUniformValueByIndex(3));
			mSDAWebsocket->changeFloatValue(2, mSDAAnimation->getFloatUniformValueByIndex(3));
			mSDAWebsocket->changeFloatValue(5, mSDAAnimation->getFloatUniformValueByIndex(7));
			mSDAWebsocket->changeFloatValue(6, mSDAAnimation->getFloatUniformValueByIndex(7));
		}

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
		mSDAWebsocket->changeFloatValue(31, getFreq(0), true);
		mSDAWebsocket->changeFloatValue(32, getFreq(1), true);
		mSDAWebsocket->changeFloatValue(33, getFreq(2), true);
		mSDAWebsocket->changeFloatValue(34, getFreq(3), true);
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
			if (settings.hasChild("bpm")) { mOriginalBpm = settings.getValueForKey<float>("bpm", 166.0f); mSDAAnimation->setBpm(mOriginalBpm); };
			if (settings.hasChild("beatsperbar")) mSDAAnimation->setIntUniformValueByName("iBeatsPerBar", settings.getValueForKey<int>("beatsperbar"));
			if (mSDAAnimation->getIntUniformValueByName("iBeatsPerBar") < 1) mSDAAnimation->setIntUniformValueByName("iBeatsPerBar", 4);
			if (settings.hasChild("fadeindelay")) mFadeInDelay = settings.getValueForKey<int>("fadeindelay");
			if (settings.hasChild("fadeoutdelay")) mFadeOutDelay = settings.getValueForKey<int>("fadeoutdelay");
			if (settings.hasChild("endframe")) mSDAAnimation->mEndFrame = settings.getValueForKey<int>("endframe");
			mTargetFps = mSDAAnimation->getBpm() / 60.0f * mFpb;
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
	mFlipV = false;
	mFlipH = false;
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
	string fileName = "";

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
		fileName = absolutePath.substr(slashIndex + 1, dotIndex - slashIndex - 1);


		if (ext == "wav" || ext == "mp3") {
			loadAudioFile(absolutePath);
		}
		else if (ext == "png" || ext == "jpg") {
			if (index < 1) index = 1;
			if (index > 3) index = 3;
			loadImageFile(absolutePath, index);
		}
		else if (ext == "glsl" || ext == "frag") {
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
				mSDAAnimation->load();
				//mLoopVideo = !mLoopVideo;
				//if (mMovie) mMovie->setLoop(mLoopVideo);
				break;
			case KeyEvent::KEY_x:
				// trixels
				mSDAWebsocket->changeFloatValue(16, mSDAAnimation->getFloatUniformValueByIndex(16) + 0.05f);
				break;
			case KeyEvent::KEY_r:
				newValue = mSDAAnimation->getFloatUniformValueByIndex(1) + 0.1f;
				if (newValue > 1.0f) newValue = 0.0f;
				mSDAWebsocket->changeFloatValue(1, newValue);
				break;
			case KeyEvent::KEY_g:
				newValue = mSDAAnimation->getFloatUniformValueByIndex(2) + 0.1f;
				if (newValue > 1.0f) newValue = 0.0f;
				mSDAWebsocket->changeFloatValue(2, newValue);
				break;
			case KeyEvent::KEY_b:
				newValue = mSDAAnimation->getFloatUniformValueByIndex(3) + 0.1f;
				if (newValue > 1.0f) newValue = 0.0f;
				mSDAWebsocket->changeFloatValue(3, newValue);
				break;
			case KeyEvent::KEY_e:
				newValue = mSDAAnimation->getFloatUniformValueByIndex(1) - 0.1f;
				if (newValue < 0.0f) newValue = 1.0;
				mSDAWebsocket->changeFloatValue(1, newValue);
				break;
			case KeyEvent::KEY_f:
				newValue = mSDAAnimation->getFloatUniformValueByIndex(2) - 0.1f;
				if (newValue < 0.0f) newValue = 1.0;
				mSDAWebsocket->changeFloatValue(2, newValue);
				break;
			case KeyEvent::KEY_v:
				newValue = mSDAAnimation->getFloatUniformValueByIndex(3) - 0.1f;
				if (newValue < 0.0f) newValue = 1.0;
				mSDAWebsocket->changeFloatValue(3, newValue);
				break;
			case KeyEvent::KEY_c:
				// chromatic
				mSDAWebsocket->changeFloatValue(10, mSDAAnimation->getFloatUniformValueByIndex(10) + 0.05f);
				break;
			case KeyEvent::KEY_p:
				// pixelate
				mSDAWebsocket->changeFloatValue(15, mSDAAnimation->getFloatUniformValueByIndex(15) + 0.05f);
				break;
			case KeyEvent::KEY_t:
				// glitch
				mSDAWebsocket->changeBoolValue(45, true);
				break;
			case KeyEvent::KEY_i:
				// invert
				mSDAWebsocket->changeBoolValue(48, true);
				break;
			case KeyEvent::KEY_o:
				// toggle
				mSDAWebsocket->toggleValue(46);
				break;
			case KeyEvent::KEY_z:
				// zoom
				mSDAWebsocket->changeFloatValue(12, mSDAAnimation->getFloatUniformValueByIndex(12) - 0.05f);
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
				if (mSDAAnimation->getFloatUniformValueByIndex(18) < 1.0f) mSDAWebsocket->changeFloatValue(21, mSDAAnimation->getFloatUniformValueByIndex(18) + 0.1f);
				break;
			case KeyEvent::KEY_PAGEUP:
				// crossfade left
				if (mSDAAnimation->getFloatUniformValueByIndex(18) > 0.0f) mSDAWebsocket->changeFloatValue(21, mSDAAnimation->getFloatUniformValueByIndex(18) - 0.1f);
				break;

			default:
				handled = false;
				break;
			}

	}
	event.setHandled(handled);
	return event.isHandled();
}
bool SDASession::handleKeyUp(KeyEvent &event) {
	bool handled = true;


		if (!mSDAAnimation->handleKeyUp(event)) {
			// Animation did not handle the key, so handle it here
			switch (event.getCode()) {
			case KeyEvent::KEY_g:
				// glitch
				mSDAWebsocket->changeBoolValue(45, false);
				break;
			case KeyEvent::KEY_t:
				// trixels
				mSDAWebsocket->changeFloatValue(16, 0.0f);
				break;
			case KeyEvent::KEY_i:
				// invert
				mSDAWebsocket->changeBoolValue(48, false);
				break;
			case KeyEvent::KEY_c:
				// chromatic
				mSDAWebsocket->changeFloatValue(10, 0.0f);
				break;
			case KeyEvent::KEY_p:
				// pixelate
				mSDAWebsocket->changeFloatValue(15, 1.0f);
				break;
			case KeyEvent::KEY_o:
				// toggle
				mSDAWebsocket->changeBoolValue(46, false);
				break;
			case KeyEvent::KEY_z:
				// zoom
				mSDAWebsocket->changeFloatValue(12, 1.0f);
				break;
			default:
				handled = false;
				break;
			}
		
	}
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
	/*mSDAMix->setWarpAFboIndex(aIndex, aFboIndex);
	mSDARouter->setWarpAFboIndex(aIndex, aFboIndex);
	mSDAWebsocket->changeWarpFboIndex(aIndex, aFboIndex, 0);*/
}
void SDASession::setFboBIndex(unsigned int aIndex, unsigned int aFboIndex) {
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

/*string SDASession::getVertexShaderString(unsigned int aShaderIndex) {
	if (aShaderIndex > mShaderList.size() - 1) aShaderIndex = mShaderList.size() - 1;
	return mShaderList[aShaderIndex]->getVertexString();
}*/


unsigned int SDASession::createShaderFbo(string aShaderFilename, unsigned int aInputTextureIndex) {
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
		createShaderFboFromString("void main(void){vec2 uv = gl_FragCoord.xy / iResolution.xy;fragColor = vec4(sin(uv.x), sin(uv.y), 0.0, 1.0);}", "tex1");
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

string SDASession::getMixFboName(unsigned int aMixFboIndex) {
	if (aMixFboIndex > mMixFbos.size() - 1) aMixFboIndex = mMixFbos.size() - 1;
	mMixFbos[aMixFboIndex].name = mFboList[0]->getShaderName() + "/" + mFboList[1]->getShaderName();
	return mMixFbos[aMixFboIndex].name;
}

