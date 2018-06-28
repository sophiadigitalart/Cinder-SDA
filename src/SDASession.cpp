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

	// Mix
	mSDAMix = SDAMix::create(mSDASettings, mSDAAnimation);
	// Websocket
	mSDAWebsocket = SDAWebsocket::create(mSDASettings, mSDAAnimation, mSDAMix);
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
			mSDAMix->createShaderFbo(fboChild->getAttributeValue<string>("shadername", ""), 0);
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

void SDASession::resize() {
	mSDAMix->resize();
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
		mSDAMix->update();
		mSDAAnimation->update();

	}
	else {
		// aClassIndex == 1 (audio analysis only)
		mSDAMix->updateAudio();
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
	// save warp settings
	mSDAMix->save();
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
	mSDAMix->load();

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

	unsigned int index = (int)(event.getX() / (mSDASettings->uiLargePreviewW + mSDASettings->uiMargin));
	int y = (int)(event.getY());
	if (index < 2 || y < mSDASettings->uiYPosRow3 || y > mSDASettings->uiYPosRow3 + mSDASettings->uiPreviewH) index = 0;
	ci::fs::path mPath = event.getFile(event.getNumFiles() - 1);
	string absolutePath = mPath.string();
	// use the last of the dropped files
	int dotIndex = absolutePath.find_last_of(".");
	int slashIndex = absolutePath.find_last_of("\\");

	if (dotIndex != std::string::npos && dotIndex > slashIndex) ext = absolutePath.substr(absolutePath.find_last_of(".") + 1);

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
#pragma region events
bool SDASession::handleMouseMove(MouseEvent &event)
{
	bool handled = true;
	// 20180318 handled in SDAUIMouse mSDAAnimation->setVec4UniformValueByIndex(70, vec4(event.getX(), event.getY(), event.isLeftDown(), event.isRightDown()));
	// pass this mouse event to the warp editor first
	if (!mSDAMix->handleMouseMove(event)) {
		// let your application perform its mouseMove handling here
		handled = false;
	}
	event.setHandled(handled);
	return event.isHandled();
}

bool SDASession::handleMouseDown(MouseEvent &event)
{
	bool handled = true;
	// 20180318 handled in SDAUIMouse mSDAAnimation->setVec4UniformValueByIndex(70, vec4(event.getX(), event.getY(), event.isLeftDown(), event.isRightDown()));
	// pass this mouse event to the warp editor first
	if (!mSDAMix->handleMouseDown(event)) {
		// let your application perform its mouseDown handling here
		mSDAWebsocket->changeFloatValue(21, event.getX() / getWindowWidth());
		handled = false;
	}
	event.setHandled(handled);
	return event.isHandled();
}

bool SDASession::handleMouseDrag(MouseEvent &event)
{
	bool handled = true;
	// 20180318 handled in SDAUIMouse mSDAAnimation->setVec4UniformValueByIndex(70, vec4(event.getX(), event.getY(), event.isLeftDown(), event.isRightDown()));
	// pass this mouse event to the warp editor first
	if (!mSDAMix->handleMouseDrag(event)) {
		// let your application perform its mouseDrag handling here
		handled = false;
	}
	event.setHandled(handled);
	return event.isHandled();
}

bool SDASession::handleMouseUp(MouseEvent &event)
{
	bool handled = true;
	// 20180318 handled in SDAUIMouse mSDAAnimation->setVec4UniformValueByIndex(70, vec4(event.getX(), event.getY(), event.isLeftDown(), event.isRightDown()));
	// pass this mouse event to the warp editor first
	if (!mSDAMix->handleMouseUp(event)) {
		// let your application perform its mouseUp handling here
		handled = false;
	}
	event.setHandled(handled);
	return event.isHandled();
}

bool SDASession::handleKeyDown(KeyEvent &event)
{
	bool handled = true;
	float newValue;

	// pass this key event to the warp editor first
	if (!mSDAMix->handleKeyDown(event)) {
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
	}
	event.setHandled(handled);
	return event.isHandled();
}
bool SDASession::handleKeyUp(KeyEvent &event) {
	bool handled = true;

	// pass this key event to the warp editor first
	if (!mSDAMix->handleKeyUp(event)) {
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
	}
	event.setHandled(handled);
	return event.isHandled();
}
#pragma endregion events
// fbos
#pragma region fbos
bool SDASession::isFlipH() {
	return mSDAAnimation->isFlipH();
}
bool SDASession::isFlipV() {
	return mSDAAnimation->isFlipV();
}
void SDASession::flipH() {
	mSDAAnimation->flipH();
}
void SDASession::flipV() {
	mSDAAnimation->flipV();
}

ci::gl::TextureRef SDASession::getMixTexture(unsigned int aMixFboIndex) {
	return mSDAMix->getMixTexture(aMixFboIndex);
}
int SDASession::loadFragmentShader(string aFilePath) {
	int rtn = -1;
	CI_LOG_V("loadFragmentShader " + aFilePath);
	mSDAMix->createShaderFbo(aFilePath);

	return rtn;
}

string SDASession::getMixFboName(unsigned int aMixFboIndex) {
	return mSDAMix->getMixFboName(aMixFboIndex);
}

void SDASession::sendFragmentShader(unsigned int aShaderIndex) {
	mSDAWebsocket->changeFragmentShader(mSDAMix->getFragmentString(aShaderIndex));
}

unsigned int SDASession::getMixFbosCount() {
	return mSDAMix->getMixFbosCount();
};
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