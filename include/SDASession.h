#pragma once

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
// json
#include "cinder/Json.h"

// Settings
#include "SDASettings.h"
// Utils
#include "SDAUtils.h"
// Message router
#include "SDARouter.h"
// Websocket
#include "SDAWebsocket.h"
// Animation
#include "SDAAnimation.h"
// Fbos
#include "SDAFbo.h"
// Logger
#include "SDALog.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace SophiaDigitalArt {

	typedef std::shared_ptr<class SDASession> SDASessionRef;

	struct SDAMixFbo
	{
		ci::gl::FboRef					fbo;
		ci::gl::Texture2dRef			texture;
		string							name;
	};
	class SDASession {
	public:
		SDASession(SDASettingsRef aSDASettings);
		static SDASessionRef				create(SDASettingsRef aSDASettings);

		//!
		void							fromXml(const ci::XmlTree &xml);
		//!
		//XmlTree							toXml() const;
		//! read a xml file and pass back a vector of SDAMixs
		void							readSettings(SDASettingsRef aSDASettings, SDAAnimationRef aSDAAnimation, const ci::DataSourceRef &source);
		//! write a xml file
		//static void						writeSettings(const SDAMixList &SDAMixlist, const ci::DataTargetRef &target);

		bool							handleMouseMove(MouseEvent &event);
		bool							handleMouseDown(MouseEvent &event);
		bool							handleMouseDrag(MouseEvent &event);
		bool							handleMouseUp(MouseEvent &event);
		bool							handleKeyDown(KeyEvent &event);
		bool							handleKeyUp(KeyEvent &event);
		void							resize(){mRenderFbo = gl::Fbo::create(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight, fboFmt);}
		void							update(unsigned int aClassIndex = 0);
		bool							save();
		void							restore();
		void							reset();
		void							resetSomeParams();

		string							getWaveFileName() { return mWaveFileName; };
		int								getWavePlaybackDelay() { return mWavePlaybackDelay; };
		string							getMovieFileName() { return mMovieFileName; };
		int								getMoviePlaybackDelay() { return mMoviePlaybackDelay; };
		bool							hasMovie() { return mMovieFileName.length() > 0; };
		unsigned int					getFadeInDelay() { 
			return mFadeInDelay; 
		};
		unsigned int					getFadeOutDelay() { return mFadeOutDelay; };
		string							getImageSequencePath() { return mImageSequencePath; };
		bool							hasImageSequencePath() { return mImageSequencePath.length() > 0; };
		string							getText() { return mText; };
		int								getTextStart() { return mTextPlaybackDelay; };
		int								getTextEnd() { return mTextPlaybackEnd; };
		bool							hasText() { return mText.length() > 0; };
		// control values
		void							toggleValue(unsigned int aCtrl);
		void							toggleAuto(unsigned int aCtrl);
		void							toggleTempo(unsigned int aCtrl);

		void							resetAutoAnimation(unsigned int aIndex);
		float							getMinUniformValueByIndex(unsigned int aIndex);
		float							getMaxUniformValueByIndex(unsigned int aIndex);
		vec2							getVec2UniformValueByIndex(unsigned int aIndex) {
			return mSDAAnimation->getVec2UniformValueByIndex(aIndex);
		};
		vec3							getVec3UniformValueByIndex(unsigned int aIndex) {
			return mSDAAnimation->getVec3UniformValueByIndex(aIndex);
		};
		vec4							getVec4UniformValueByIndex(unsigned int aIndex) {
			return mSDAAnimation->getVec4UniformValueByIndex(aIndex);
		};
		int								getSampler2DUniformValueByName(string aName) {
			return mSDAAnimation->getSampler2DUniformValueByName(aName);
		};
		vec2							getVec2UniformValueByName(string aName) {
			return mSDAAnimation->getVec2UniformValueByName(aName);
		};
		vec3							getVec3UniformValueByName(string aName) {
			return mSDAAnimation->getVec3UniformValueByName(aName);
		};
		vec4							getVec4UniformValueByName(string aName) {
			return mSDAAnimation->getVec4UniformValueByName(aName);
		};
		int								getIntUniformValueByName(string aName) {
			return mSDAAnimation->getIntUniformValueByName(aName);
		};
		bool							getBoolUniformValueByName(string aName) {
			return mSDAAnimation->getBoolUniformValueByName(aName);
		};
		bool							getBoolUniformValueByIndex(unsigned int aCtrl) {
			return mSDAAnimation->getBoolUniformValueByIndex(aCtrl);
		}
		float							getFloatUniformValueByIndex(unsigned int aCtrl) {
			return mSDAAnimation->getFloatUniformValueByIndex(aCtrl);
		};
		float							getFloatUniformValueByName(string aCtrlName) {
			return mSDAAnimation->getFloatUniformValueByName(aCtrlName);
		};
		void							setFloatUniformValueByIndex(unsigned int aCtrl, float aValue) {
			// done in router mSDAAnimation->changeFloatValue(aCtrl, aValue);
			mSDAWebsocket->changeFloatValue(aCtrl, aValue);
		};

		// tempo
		float							getBpm() { return mSDAAnimation->getBpm(); };
		void							setBpm(float aBpm) { mSDAAnimation->setBpm(aBpm); };
		void							tapTempo() { mSDAAnimation->tapTempo(); };
		// audio
		float							getMaxVolume() { return mSDAAnimation->maxVolume; };
		float *							getFreqs() { return mSDAAnimation->iFreqs; };
		int								getFreqIndexSize() { return mSDAAnimation->getFreqIndexSize(); };
		float							getFreq(unsigned int aFreqIndex) { return mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IFREQ0 + aFreqIndex); };
		int								getFreqIndex(unsigned int aFreqIndex) { return mSDAAnimation->getFreqIndex(aFreqIndex); };
		void							setFreqIndex(unsigned int aFreqIndex, unsigned int aFreq) { mSDAAnimation->setFreqIndex(aFreqIndex, aFreq); };
		int								getWindowSize() { return mSDAAnimation->mWindowSize; };
		bool							isAudioBuffered() { return mSDAAnimation->isAudioBuffered(); };
		void							toggleAudioBuffered() { mSDAAnimation->toggleAudioBuffered(); };
		bool							getUseLineIn() { return mSDAAnimation->getUseLineIn(); };
		void							setUseLineIn(bool useLineIn) { mSDAAnimation->setUseLineIn(useLineIn); };
		void							toggleUseLineIn() { mSDAAnimation->toggleUseLineIn(); };
		bool							getFreqWSSend() { return mFreqWSSend; };
		void							toggleFreqWSSend() { mFreqWSSend = !mFreqWSSend; };
		// uniforms
		//void							setMixCrossfade(unsigned int aWarpIndex, float aCrossfade) { mSDASettings->xFade = aCrossfade; mSDASettings->xFadeChanged = true; };
		//float							getMixCrossfade(unsigned int aWarpIndex) { return mSDASettings->xFade; };
		float							getCrossfade() {
			return mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IXFADE);
		};
		void							setCrossfade(float aCrossfade) {
			mSDAAnimation->setFloatUniformValueByIndex(mSDASettings->IXFADE, aCrossfade);
		};
		void							setFboAIndex(unsigned int aIndex, unsigned int aFboIndex);
		void							setFboBIndex(unsigned int aIndex, unsigned int aFboIndex);
		unsigned int					getFboAIndex(unsigned int aIndex) { return mAFboIndex; };
		unsigned int					getFboBIndex(unsigned int aIndex) { return mBFboIndex; };

		void							setFboFragmentShaderIndex(unsigned int aFboIndex, unsigned int aFboShaderIndex);
		unsigned int					getFboFragmentShaderIndex(unsigned int aFboIndex);
		bool							loadShaderFolder(string aFolder);
		int								loadFragmentShader(string aFilePath);
		unsigned int					getShadersCount() { return mShaderList.size(); };
		string							getShaderName(unsigned int aShaderIndex);
		ci::gl::TextureRef				getShaderThumb(unsigned int aShaderIndex);
		string							getFragmentString(unsigned int aShaderIndex) { return mShaderList[aShaderIndex]->getFragmentString(); };
		void							setFragmentShaderString(unsigned int aShaderIndex, string aFragmentShaderString, string aName = "");
		//string							getVertexShaderString(unsigned int aShaderIndex) { return mSDAMix->getVertexShaderString(aShaderIndex); };
		string							getFragmentShaderString(unsigned int aShaderIndex);
		void							updateShaderThumbFile(unsigned int aShaderIndex);
		void							removeShader(unsigned int aShaderIndex);
		// utils
		int								getWindowsResolution();
		float							getTargetFps() { return mTargetFps; };
		void							blendRenderEnable(bool render);

		// file operations (filedrop, etc)
		//int								loadFileFromAbsolutePath(string aAbsolutePath, int aIndex = 0);
		void							fileDrop(FileDropEvent event);

		// fbos
		void							fboFlipV(unsigned int aFboIndex) ;
		bool							isFboFlipV(unsigned int aFboIndex) ;
		unsigned int					getFboInputTextureIndex(unsigned int aFboIndex) ;
		void							setFboInputTexture(unsigned int aFboIndex, unsigned int aInputTextureIndex);
		ci::gl::TextureRef				getFboTexture(unsigned int aFboIndex = 0);
		ci::gl::TextureRef				getFboRenderedTexture(unsigned int aFboIndex);
		ci::gl::TextureRef				getFboThumb(unsigned int aBlendIndex) { return mBlendFbos[aBlendIndex]->getColorTexture(); };
		unsigned int 					createShaderFbo(string aShaderFilename, unsigned int aInputTextureIndex = 0);
		unsigned int					createShaderFboFromString(string aFragmentShaderString, string aShaderFilename);
		string							getFboName(unsigned int aFboIndex) { return mFboList[aFboIndex]->getName(); };
		//int								getFboTextureWidth(unsigned int aFboIndex);
		//int								getFboTextureHeight(unsigned int aFboIndex);
		unsigned int					getFboListSize() { return mFboList.size(); };
		//string							getFboFragmentShaderText(unsigned int aFboIndex);
		// feedback get/set
		/*int								getFeedbackFrames() {
			return mSDAMix->getFeedbackFrames();
		};
		void							setFeedbackFrames(int aFeedbackFrames) {
			mSDAMix->setFeedbackFrames(aFeedbackFrames);
		};*/
		string							getMixFboName(unsigned int aMixFboIndex);
		ci::gl::TextureRef				getMixTexture(unsigned int aMixFboIndex = 0);
		unsigned int					getMixFbosCount() { return mMixFbos.size(); };
		// RTE in release mode ci::gl::Texture2dRef			getRenderedTexture(bool reDraw = true) { return mSDAMix->getRenderedTexture(reDraw); };
		ci::gl::TextureRef				getRenderTexture();
		bool							isEnabledAlphaBlending() { return mEnabledAlphaBlending; };
		void							toggleEnabledAlphaBlending() { mEnabledAlphaBlending = !mEnabledAlphaBlending; }
		bool							isRenderTexture() { return mRenderTexture; };
		void							toggleRenderTexture() { mRenderTexture = !mRenderTexture; }
		bool							isAutoLayout() { return mSDASettings->mAutoLayout; };
		void							toggleAutoLayout() { mSDASettings->mAutoLayout = !mSDASettings->mAutoLayout; }
		bool							isFlipH() { return mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IFLIPH); };
		bool							isFlipV() { return mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IFLIPV); };
		void							flipH(){mSDAAnimation->setBoolUniformValueByIndex(mSDASettings->IFLIPH, !mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IFLIPH));};
		void							flipV(){ mSDAAnimation->setBoolUniformValueByIndex(mSDASettings->IFLIPV, !mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IFLIPV));};

		// blendmodes
		unsigned int					getFboBlendCount() { return mBlendFbos.size(); };
		void							useBlendmode(unsigned int aBlendIndex) { mSDASettings->iBlendmode = aBlendIndex; };

		// textures
		ci::gl::TextureRef				getInputTexture(unsigned int aTextureIndex);
		string							getInputTextureName(unsigned int aTextureIndex);
		unsigned int					getInputTexturesCount();
		void							loadImageFile(string aFile, unsigned int aTextureIndex);
		void							loadAudioFile(string aFile);
		void							loadMovie(string aFile, unsigned int aTextureIndex);
		bool							loadImageSequence(string aFolder, unsigned int aTextureIndex);
		//void							toggleSharedOutput(unsigned int aMixFboIndex = 0);
		//bool							isSharedOutputActive() { return mSDAMix->isSharedOutputActive(); };
		//unsigned int					getSharedMixIndex() { return mSDAMix->getSharedMixIndex(); };
		// move, rotate, zoom methods
		//void							setPosition(int x, int y);
		//void							setZoom(float aZoom);
		int								getInputTextureXLeft(unsigned int aTextureIndex);
		void							setInputTextureXLeft(unsigned int aTextureIndex, int aXLeft);
		int								getInputTextureYTop(unsigned int aTextureIndex);
		void							setInputTextureYTop(unsigned int aTextureIndex, int aYTop);
		int								getInputTextureXRight(unsigned int aTextureIndex);
		void							setInputTextureXRight(unsigned int aTextureIndex, int aXRight);
		int								getInputTextureYBottom(unsigned int aTextureIndex);
		void							setInputTextureYBottom(unsigned int aTextureIndex, int aYBottom);
		bool							isFlipVInputTexture(unsigned int aTextureIndex);
		bool							isFlipHInputTexture(unsigned int aTextureIndex);
		void							inputTextureFlipV(unsigned int aTextureIndex);
		void							inputTextureFlipH(unsigned int aTextureIndex);
		bool							getInputTextureLockBounds(unsigned int aTextureIndex);
		void							toggleInputTextureLockBounds(unsigned int aTextureIndex);
		unsigned int					getInputTextureOriginalWidth(unsigned int aTextureIndex);
		unsigned int					getInputTextureOriginalHeight(unsigned int aTextureIndex);
		void							togglePlayPause(unsigned int aTextureIndex);
		// movie
		bool							isMovie(unsigned int aTextureIndex);
		// sequence
		bool							isSequence(unsigned int aTextureIndex);
		bool							isLoadingFromDisk(unsigned int aTextureIndex);
		void							toggleLoadingFromDisk(unsigned int aTextureIndex);
		void							syncToBeat(unsigned int aTextureIndex);
		void							reverse(unsigned int aTextureIndex);
		float							getSpeed(unsigned int aTextureIndex);
		void							setSpeed(unsigned int aTextureIndex, float aSpeed);
		int								getPosition(unsigned int aTextureIndex);
		void							setPlayheadPosition(unsigned int aTextureIndex, int aPosition);
		int								getMaxFrame(unsigned int aTextureIndex);
		// websockets
		void							wsConnect();
		void							wsPing();
		void							wsWrite(std::string msg);
		void							sendFragmentShader(unsigned int aShaderIndex);
		// midi
		void							midiSetup() { mSDARouter->midiSetup(); };
		void							midiOutSendNoteOn(int i, int channel, int pitch, int velocity) { mSDARouter->midiOutSendNoteOn(i, channel, pitch, velocity); };

		int								getMidiInPortsCount() { return mSDARouter->getMidiInPortsCount(); };
		string							getMidiInPortName(int i) { return mSDARouter->getMidiInPortName(i); };
		bool							isMidiInConnected(int i) { return mSDARouter->isMidiInConnected(i); };
		int								getMidiOutPortsCount() { return mSDARouter->getMidiOutPortsCount(); };
		string							getMidiOutPortName(int i) { return mSDARouter->getMidiOutPortName(i); };
		bool							isMidiOutConnected(int i) { return mSDARouter->isMidiOutConnected(i); };
		void							openMidiInPort(int i) { mSDARouter->openMidiInPort(i); };
		void							closeMidiInPort(int i) { mSDARouter->closeMidiInPort(i); };
		void							openMidiOutPort(int i) { mSDARouter->openMidiOutPort(i); };
		void							closeMidiOutPort(int i) { mSDARouter->closeMidiOutPort(i); };
		//! window management
		void							createWindow() { cmd = 0; };
		void							deleteWindow() { cmd = 1; };
		int								getCmd() { int rtn = cmd; cmd = -1; return rtn; };
		// utils
		float							formatFloat(float f) { return mSDAUtils->formatFloat(f); };
		
		void							load();
		void							updateAudio() {mTextureList[0]->getTexture();}
		void							updateMixUniforms();
		void							updateBlendUniforms();
	private:
		// Settings
		SDASettingsRef					mSDASettings;
		// Utils
		SDAUtilsRef						mSDAUtils;
		// Message router
		SDARouterRef					mSDARouter;
		// SDAWebsocket
		SDAWebsocketRef					mSDAWebsocket;
		// Animation
		SDAAnimationRef					mSDAAnimation;
		// Log
		SDALogRef						mSDALog;

		const string					sessionFileName = "session.json";
		fs::path						sessionPath;
		// tempo
		float							mFpb;
		float							mOriginalBpm;
		float							mTargetFps;
		// audio
		bool							mFreqWSSend;
		// files and paths
		string							mWaveFileName;
		string							mMovieFileName;
		string							mImageSequencePath;
		// delay
		int								mWavePlaybackDelay;
		int								mMoviePlaybackDelay;
		unsigned int					mFadeInDelay;
		unsigned int					mFadeOutDelay;
		// font and text 
		string							mText;
		int								mTextPlaybackDelay;
		int								mTextPlaybackEnd;
		//! Fbos
		// maintain a list of fbo for right only or left/right or more fbos specific to this mix
		//SDAFboList						mFboList;
		fs::path						mFbosFilepath;
		// fbo 
		//bool							mFlipV;
		//bool							mFlipH;
		gl::Texture::Format				fmt;
		gl::Fbo::Format					fboFmt;
		bool							mEnabledAlphaBlending;
		bool							mRenderTexture;
		//! Warps
		int								mSelectedWarp;
		//! Shaders
		string							mShaderLeft;
		string							mShaderRight;
		//! textures
		int								mWidth;
		int								mHeight;
		float							mPosX;
		float							mPosY;
		float							mZoom;
		void							updateStream(string * aStringPtr);
		//! window management
		int								cmd;

		/* 
			mix
		*/

		std::string						mFbosPath;

		//! mix shader
		gl::GlslProgRef					mMixShader;
		string							mError;

		//! Fbos
		map<int, SDAMixFbo>				mMixFbos;
		//map<int, SDAMixFbo>				mTriangleFbos;
		// maintain a list of fbos specific to this mix
		SDAFboList						mFboList;
		fs::path						mMixesFilepath;
		unsigned int					mAFboIndex;
		unsigned int					mBFboIndex;

		//! Shaders
		SDAShaderList					mShaderList;
		void							initShaderList();
		//! Textures
		SDATextureList					mTextureList;
		fs::path						mTexturesFilepath;
		bool							initTextureList();
		// blendmodes fbos
		map<int, ci::gl::FboRef>		mBlendFbos;
		int								mCurrentBlend;
		gl::GlslProgRef					mGlslMix, mGlslBlend, mGlslFeedback;
		// render
		void							renderMix();
		void							renderBlend();
		// warping
		gl::FboRef						mRenderFbo;
		// warp rendered texture
		ci::gl::Texture2dRef			mRenderedTexture;
	};

}
