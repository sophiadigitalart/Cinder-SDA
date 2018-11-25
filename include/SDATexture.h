#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Xml.h"
#include "cinder/Json.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "cinder/Timeline.h"

#if defined( CINDER_MSW )
// spout
#include "CiSpoutIn.h"
#endif

#if defined( NDI_RECEIVER )
// ndi
#include "CinderNDIReceiver.h"
#endif

#if defined( CINDER_MAC )
// syphon
#include "cinderSyphon.h"
#endif

// Settings
#include "SDAAnimation.h"

// audio
#include "cinder/audio/Context.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"
#include "cinder/audio/Source.h"
#include "cinder/audio/Target.h"
#include "cinder/audio/dsp/Converter.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/SampleRecorderNode.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/Rand.h"
// base64 for stream
#include "cinder/Base64.h"

#include <atomic>
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;


namespace SophiaDigitalArt
{
	/*
	** ---- Texture parent class ------------------------------------------------
	*/
	// stores the pointer to the SDATexture instance
	typedef std::shared_ptr<class SDATexture> 	SDATextureRef;
	typedef std::vector<SDATextureRef>			SDATextureList;
	// for profiling
	typedef std::chrono::high_resolution_clock Clock;

	class SDATexture : public std::enable_shared_from_this < SDATexture > {
	public:
		typedef enum { UNKNOWN, IMAGE, SEQUENCE, CAMERA, SHARED, AUDIO, STREAM } TextureType;
	public:
		SDATexture(TextureType aType = UNKNOWN);
		virtual ~SDATexture(void);

		virtual ci::gl::Texture2dRef	getTexture();
		//! returns a shared pointer to this input texture
		SDATextureRef					getPtr() { return shared_from_this(); }
		ci::ivec2						getSize();
		ci::Area						getBounds();
		GLuint							getId();
		//! returns the type
		TextureType						getType() { return mType; };
		std::string						getName();
		unsigned int					getTextureWidth();
		unsigned int					getTextureHeight();
		unsigned int					getOriginalWidth();
		unsigned int					getOriginalHeight();
		//!
		virtual bool					fromXml(const ci::XmlTree &xml);
		//!
		virtual XmlTree					toXml() const;
		//! read a xml file and pass back a vector of SDATextures
		static SDATextureList			readSettings(SDAAnimationRef aSDAAnimation, const ci::DataSourceRef &source);
		//! write a xml file
		static void						writeSettings(const SDATextureList &vdtexturelist, const ci::DataTargetRef &target);
		virtual bool					loadFromFullPath(string aPath);
		string							getStatus() { return mStatus; };
		//! area to display
		void							lockBounds(bool lock, unsigned int aWidth, unsigned int aHeight);
		void							setXLeft(int aXleft);
		void							setYTop(int aYTop);
		void							setXRight(int aXRight);
		void							setYBottom(int aYBottom);
		int								getXLeft() { return mXLeft; };
		int								getYTop() { return mYTop; };
		int								getXRight() { return mXRight; };
		int								getYBottom() { return mYBottom; };
		bool							isFlipH() { return mFlipH; };
		bool							isFlipV() { return mFlipV; };
		void							flipV();
		void							flipH();
		bool							getLockBounds();
		void							toggleLockBounds();
		// sequence and movie
		void							togglePlayPause();
		// sequence only
		virtual void					toggleLoadingFromDisk();
		virtual bool					isLoadingFromDisk();
		void							syncToBeat();
		virtual void					reverse();
		virtual float					getSpeed();
		virtual void					setSpeed(float speed);
		//virtual int						getPlayheadPosition();
		int								getPosition() { return mPosition; };
		virtual void					setPlayheadPosition(int position);
		virtual int						getMaxFrame();

	protected:
		std::string						mName;
		bool							mFlipV;
		bool							mFlipH;
		TextureType						mType;
		std::string						mPath;
		std::string						mFolder;
		unsigned int 					mWidth;
		unsigned int					mHeight;
		unsigned int 					mAreaWidth;
		unsigned int					mAreaHeight;
		int								mPosition;
		std::string						mStatus;
		//! Texture
		ci::gl::Texture2dRef			mTexture;
		//! Surface
		Surface							mInputSurface;
		Surface							mProcessedSurface;
		int								mXLeft, mYTop, mXRight, mYBottom, mOriginalWidth, mOriginalHeight;
		bool							mBoundsLocked;
		bool							mSyncToBeat;
		bool							mPlaying;
		//! Fbo for audio only for now
		gl::FboRef						mFbo;
		gl::Texture::Format				fmt;
		gl::Fbo::Format					fboFmt;
		ci::gl::Texture2dRef			mRenderedTexture;

	private:
	};
	/*
	** ---- TextureImage ------------------------------------------------
	*/
	typedef std::shared_ptr<class TextureImage>	TextureImageRef;

	class TextureImage
		: public SDATexture {
	public:
		//
		static TextureImageRef	create() { return std::make_shared<TextureImage>(); }
		//!
		bool					fromXml(const XmlTree &xml) override;
		//!
		virtual	XmlTree			toXml() const override;
		virtual bool			loadFromFullPath(string aPath) override;

	public:
		TextureImage();
		virtual ~TextureImage(void);

		//! returns a shared pointer
		TextureImageRef	getPtr() { return std::static_pointer_cast<TextureImage>(shared_from_this()); }
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture() override;
	};

	/*
	** ---- TextureImageSequence ------------------------------------------------
	*/
	typedef std::shared_ptr<class TextureImageSequence>	TextureImageSequenceRef;

	class TextureImageSequence
		: public SDATexture {
	public:
		//
		static TextureImageSequenceRef	create(SDAAnimationRef aSDAAnimation) { return std::make_shared<TextureImageSequence>(aSDAAnimation); }
		//!
		bool					fromXml(const XmlTree &xml) override;
		//!
		virtual	XmlTree			toXml() const override;
		//!
		virtual bool			loadFromFullPath(string aPath) override;
		TextureImageSequence(SDAAnimationRef aSDAAnimation);
		virtual ~TextureImageSequence(void);

		//! returns a shared pointer 
		TextureImageSequenceRef	getPtr() { return std::static_pointer_cast<TextureImageSequence>(shared_from_this()); }
		void						stopSequence();
		void						toggleLoadingFromDisk() override;
		bool						isLoadingFromDisk() override;
		//void						stopLoading();
		//int							getPlayheadPosition() override;
		void						setPlayheadPosition(int position) override;

		float						getSpeed() override;
		void						setSpeed(float speed) override;
		void						reverse() override;

		bool						isValid(){ return mFramesLoaded > 0; };
		int							getMaxFrame() override;
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture() override;

	private:
		// Animation
		SDAAnimationRef				mSDAAnimation;

		float						playheadFrameInc;
		void						loadNextImageFromDisk();
		void						updateSequence();
		bool						mIsSequence;
		string						mFolder;
		string						mPrefix;
		string						mExt;
		int							mNumberOfDigits;
		int							mNextIndexFrameToTry;
		int							mCurrentLoadedFrame;
		int							mFramesLoaded;
		//int							mPlayheadPosition;
		bool						mLoadingPaused;
		bool						mLoadingFilesComplete;
		float						mSpeed;
		vector<ci::gl::TextureRef>	mSequenceTextures;
	};

	/*
	** ---- TextureCamera ------------------------------------------------
	*/
#if (defined(  CINDER_MSW) ) || (defined( CINDER_MAC ))
	typedef std::shared_ptr<class TextureCamera>	TextureCameraRef;

	class TextureCamera
		: public SDATexture {
	public:
		//
		static TextureCameraRef create() { return std::make_shared<TextureCamera>(); }
		//!
		bool				fromXml(const XmlTree &xml) override;
		//!
		virtual	XmlTree	toXml() const override;

	public:
		TextureCamera();
		virtual ~TextureCamera(void);

		//! returns a shared pointer 
		TextureCameraRef	getPtr() { return std::static_pointer_cast<TextureCamera>(shared_from_this()); }
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture() override;
	private:
		void printDevices();
		string					mFirstCameraDeviceName;
		CaptureRef				mCapture;
	};
#endif
	/*
	** ---- TextureShared ------------------------------------------------
	*/
#if (defined(  CINDER_MSW) ) || (defined( CINDER_MAC ))
	typedef std::shared_ptr<class TextureShared>	TextureSharedRef;

	class TextureShared
		: public SDATexture {
	public:
		//
		static TextureSharedRef create() { return std::make_shared<TextureShared>(); }
		//!
		bool				fromXml(const XmlTree &xml) override;
		//!
		virtual	XmlTree	toXml() const override;

	public:
		TextureShared();
		virtual ~TextureShared(void);

		//! returns a shared pointer 
		TextureSharedRef	getPtr() { return std::static_pointer_cast<TextureShared>(shared_from_this()); }
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture() override;
	private:
#if defined( CINDER_MSW )
		// -------- SPOUT -------------
		SpoutIn							mSpoutIn;
		//CinderNDIReceiver				mReceiver;
#endif
#if defined( CINDER_MAC )
		syphonClient					mClientSyphon;
#endif
		ci::gl::Texture2dRef			mTexture;
	};
#endif
	/*
	** ---- TextureAudio ------------------------------------------------
	*/
	typedef std::shared_ptr<class TextureAudio>	TextureAudioRef;

	class TextureAudio
		: public SDATexture {
	public:
		//
		static TextureAudioRef	create(SDAAnimationRef aSDAAnimation) { return std::make_shared<TextureAudio>(aSDAAnimation); }
		//!
		bool					fromXml(const XmlTree &xml) override;
		//!
		virtual	XmlTree			toXml() const override;
		//!
		virtual bool			loadFromFullPath(string aPath) override;

	public:
		TextureAudio(SDAAnimationRef aSDAAnimation);
		virtual ~TextureAudio(void);

		//! returns a shared pointer 
		TextureAudioRef					getPtr() { return std::static_pointer_cast<TextureAudio>(shared_from_this()); }
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture() override;
		//float							getIntensity() override;
	private:
		// Animation
		SDAAnimationRef					mSDAAnimation;
		// init
		bool							mLineInInitialized;
		//void							initializeLineIn();
		//audio::Context*					audioContext;
		// audio
		audio::InputDeviceNodeRef		mLineIn;
		audio::MonitorSpectralNodeRef	mMonitorLineInSpectralNode;
		audio::MonitorSpectralNodeRef	mMonitorWaveSpectralNode;
		audio::SamplePlayerNodeRef		mSamplePlayerNode;
		audio::SourceFileRef			mSourceFile;
		audio::MonitorSpectralNodeRef	mScopeLineInFmt;
		audio::BufferPlayerNodeRef		mBufferPlayerNode;

		vector<float>					mMagSpectrum;

		// number of frequency bands of our spectrum
		static const int				kBands = 16;

		// textures
		unsigned char					dTexture[256];// MUST be < mSDAAnimation->mWindowSize
		ci::gl::Texture2dRef			mTexture;
		//WaveformPlot					mWaveformPlot;
	};

	/*
	** ---- TextureStream ------------------------------------------------
	*/
	typedef std::shared_ptr<class TextureStream>	TextureStreamRef;

	class TextureStream
		: public SDATexture {
	public:
		//
		static TextureStreamRef	create(SDAAnimationRef aSDAAnimation) { return std::make_shared<TextureStream>(aSDAAnimation); }
		//!
		bool					fromXml(const XmlTree &xml) override;
		//!
		virtual	XmlTree			toXml() const override;
		//!
		virtual bool			loadFromFullPath(string aStream) override;

	public:
		TextureStream(SDAAnimationRef aSDAAnimation);
		virtual ~TextureStream(void);

		//! returns a shared pointer 
		TextureStreamRef				getPtr() { return std::static_pointer_cast<TextureStream>(shared_from_this()); }
	protected:
		//! 
		virtual ci::gl::Texture2dRef	getTexture() override;
	private:
		// Animation
		SDAAnimationRef					mSDAAnimation;
		// textures
		ci::gl::Texture2dRef			mTexture;

	};
}
