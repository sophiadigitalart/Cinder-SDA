#pragma once

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Timeline.h"
#include "cinder/Xml.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace videodromm {

	typedef std::shared_ptr<class VDSettings> VDSettingsRef;

	class VDSettings
	{
	public:
		VDSettings(string filename);
		static VDSettingsRef create(string filename);

		enum VDParameters {
			//! maximum number of fbos, shaders, textures
			MAX = 14,
			// uniform indexes
			// float
			ITIME = 0,
			IFR = 1,
			IFG = 2,
			IFB = 3,
			IFA = 4,
			IFRX = 5,
			IFGX = 6,
			IFBX = 7,
			ISOBEL = 8,
			IROTATIONSPEED = 9,
			ISTEPS = 10,
			IRATIO = 11,
			IZOOM = 12,
			IAUDIOX = 13,
			IEXPOSURE = 14,
			IPIXELATE = 15,
			ITRIXELS = 16,
			ICHROMATIC = 17,
			IXFADE = 18,
			ITEMPOTIME = 19,
			IFPS = 20,

			IBPM = 21,
			ISPEED = 22,
			IPIXELX = 23,
			IPIXELY = 24,
			IDELTATIME = 25,
			IBR = 26,
			IBG = 27,
			IBB = 28,
			IMAXVOLUME = 29,
			ICONTOUR = 30,

			IWEIGHT0 = 31,
			IWEIGHT1 = 32,
			IWEIGHT2 = 33,
			IWEIGHT3 = 34,
			IWEIGHT4 = 35,
			IWEIGHT5 = 36,
			IWEIGHT6 = 37,
			IWEIGHT7 = 38,
			IWEIGHT8 = 39,

			IMOUSEX = 42,
			IMOUSEY = 43,
			IMOUSEZ = 44,
			IVAMOUNT = 45,
			IVFALLOFF = 46,
			TIME = 47,
			IBADTV = 48,
			ITIMEFACTOR = 49,
			// int
			IBLENDMODE = 50,
			IBEAT = 51,
			IBAR = 52,
			IBARBEAT = 53,
			IFBOA = 54,
			IFBOB = 55,
			IOUTW = 56,
			IOUTH = 57,
			IBEATSPERBAR = 59,

			ICOLOR = 61,
			IBACKGROUNDCOLOR = 62,
			IPHASE = 63,
			IELAPSED = 64,
			IMOUSE = 70,
			// booleans 80 to 110
			IGLITCH = 81,	// A4
			IVIGN = 82,		// A#4
			ITOGGLE = 83,	// B4 for sos
			IINVERT = 86,	// D5
			IGREYSCALE = 87,
			ICLEAR = 88,	// 

			IRESOLUTION = 120,
			IRESOLUTIONX = 121,
			IRESOLUTIONY = 122,

			IDEBUG = 129,
			IXORY = 130,
			IFLIPH = 131,
			IFLIPV = 132,
			IFLIPPOSTH = 133,
			IFLIPPOSTV = 134,

			/*
						IRHANDX = 110,
						IRHANDY = 111,
						IRHANDZ = 112,
						ILHANDX = 113,
						ILHANDY = 114,
						ILHANDZ = 115,*/

			IFREQ0 = 140,
			IFREQ1 = 141,
			IFREQ2 = 142,
			IFREQ3 = 143,
			// vec2
			RESOLUTION = 150, // vec2 for hydra
			RENDERSIZE = 151, // vec2 for ISF
			IDATE = 152,
			// srcarea
			SRCXLEFT = 160,
			SRCXRIGHT = 161,
			SRCYLEFT = 162,
			SRCYRIGHT = 163,

		};
		enum VDTextureModes {
			// texture modes
			TEXTUREMODEMIX = 0,				// mix two shaders
			TEXTUREMODEAUDIO = 1,			// audio spectrum
			TEXTUREMODELEFT = 2,			// left
			TEXTUREMODERIGHT = 3,			// right
			TEXTUREMODEINPUT = 4,			// spout
			TEXTUREMODESHADER = 5,			// shader
			TEXTUREMODEIMAGE = 6,			// image
			TEXTUREMODEIMAGESEQUENCE = 7,	// image sequence
			TEXTUREMODETHUMB = 8,			// thumb
			TEXTUREMODEFBO = 9,				// fbo
			TEXTUREMODETEXT = 10,			// text
			TEXTUREMODEMOVIE = 11			// movie
		};
		bool						save();
		bool						restore();
		void						reset();
		void						resetSomeParams();

		// params
		int							mPreviewWidth, mPreviewHeight, mPreviewFboWidth, mPreviewFboHeight;
		float						mAspectRatio;
		int							mMainWindowX, mMainWindowY, mMarginSmall, mMainWindowWidth, mMainWindowHeight, mFboWidth, mFboHeight;
		float						mCodeEditorWidth, mCodeEditorHeight;
		// MIDI
		bool						mMIDIEnabled;
		// OSC
		bool						mOSCEnabled;
		std::string					mOSCDestinationHost;
		uint16_t					mOSCDestinationPort;
		std::string					mOSCDestinationHost2;
		uint16_t					mOSCDestinationPort2;
		uint16_t					mOSCReceiverPort;
		std::string					mOSCMsg;
		std::string					mMsg;
		std::string					mErrorMsg;
		std::string					mShaderMsg;
		std::string					mFboMsg;
		bool						mIsOSCSender;
		static const int			mMsgLength = 150;

		// render windows
		int							mRenderWidth;
		int							mRenderHeight;
		vec2						mRenderXY, mTexMult, mLeftRenderXY, mRightRenderXY, mPreviewRenderXY, mWarp1RenderXY, mWarp2RenderXY;
		vec2						mRenderPosXY;
		vec2						mPreviewFragXY;
		vec2						mCamPosXY;
		int							mxLeft;
		int							mxRight;
		int							myLeft;
		int							myRight;
		bool						mAutoLayout;
		bool						mStandalone;

		// code editor
		vec2						mRenderCodeEditorXY;
		bool						mLiveCode;
		bool						mShowUI;
		bool						mCursorVisible;
		bool						isUIDirty;
		bool						mMIDIOpenAllInputPorts;
		int							mCount;
		string						mImageFile;

		// shader uniforms	
		float						iSpeedMultiplier;   // speed multiplier
		float						iStart = 1.0f;		// start adjustment
		float						iBarDuration = 1.0f;// between 2 bars (seconds)
		float						iChannelTime[4];
		bool						iFade;
		bool						iRepeat;
		int							iBlendmode;
		ci::Anim<float> 			iAlpha;

		string						mAssetsPath;
		bool						iShowFps;
		string						sFps;
		unsigned int				shaderEditIndex;
		// transition
		int							iTransition;
		Anim<float>					iAnim;
		double						mTransitionDuration;
		//! Time in seconds at which the transition to the next shader starts.
		double						mTransitionTime;

		// windows and params
		int							mRenderX;
		int							mRenderY;
		int							mDisplayCount;
		bool						mPreviewEnabled;
		string						mCurrentFilePath;
		// Textures
		bool						mRenderThumbs;
		int							currentSelectedIndex;

		enum VDModes {
			// modes, should be the same in App
			MODE_MIX = 0,
			MODE_IMAGE = 1,
			MODE_SHARED = 2,
			MODE_STREAM = 3,
			MODE_SHADER = 4,
			MODE_HYDRA = 5
		};

		enum VDAnims {
			ANIM_NONE = 0,
			ANIM_TIME = 1,
			ANIM_AUTO = 2,
			ANIM_BASS = 3,
			ANIM_MID = 4,
			ANIM_TREBLE = 5
		};
		/* static const int			MODE_WARP = 1;
		static const int			MODE_AUDIO = 2;
		static const int			MODE_SPHERE = 3;
		static const int			MODE_MESH = 4;
		static const int			MODE_LIVE = 5;
		static const int			MODE_ABP = 6;
		static const int			MODE_KINECT = 8; */

		// windows to create, should be the same in App and UI
		/*static const int			NONE = 0;
		static const int			RENDER_1 = 1;
		static const int			RENDER_DELETE = 5;
		static const int			MIDI_IN = 6;*/
		
		int							mWindowToCreate;
		ColorA						FPSColor;
		ColorA						ColorGreen;
		ColorA						ColorRed;
		ColorA						ColorYellow;
		ColorA						ColorPurple;
		// audio
		bool						mIsPlaying;
		bool						mUseAudio;
		bool						mUseLineIn;
		float						iSeed;

		// z EyePointZ
		float						defaultEyePointZ;
		float						minEyePointZ;
		float						maxEyePointZ;
		bool						tEyePointZ;
		bool						autoEyePointZ;

		ci::Anim<float>				mStateTransition;

		// 
		bool						mSendToOutput;
		bool						autoInvert;

		// spout
		int							mFboResolution;
		// indexes for textures
		map<int, int>				iChannels;
		int							selectedChannel;
		int							selectedWarp;
		int							mWarpCount;
		bool						mOptimizeUI;
		bool						mSplitWarpH;
		bool						mSplitWarpV;
		int							mUIZoom;
		int							mCurrentPreviewFboIndex;
		int							iTrack;
				
		// camera
		CameraPersp					mCamera;
		vec2						mCamEyePointXY;
		float						mCamEyePointZ;
		// web sockets
		bool						mIsRouter;
		bool						mAreWebSocketsEnabledAtStartup;
		bool						mIsWebSocketsServer;
		string						mWebSocketsProtocol;
		string						mWebSocketsHost;
		string						mWebSocketsRoom;
		string						mWebSocketsNickname;
		uint16_t					mWebSocketsPort;
		std::string					mWebSocketsMsg;
		// midi
		std::string					mMidiMsg;

		// info to backup in XML
		string						mInfo;
		// ableton liveOSC
		string						mTrackName;
		// imgui positions and sizes
		int							uiMargin;
		// mPreviewFboWidth 80 mPreviewFboHeight 60 margin 10 inBetween 15
		int							uiXPosCol1;
		int							uiXPosCol2;
		int							uiXPosCol3;
		//int							uiYPos;
		int							uiYPosRow1; // top
		int							uiYPosRow2; // details, textures
		int							uiYPosRow3;
		int							uiYPosRow4;
		int							uiYPosRow5;
		int							uiYPosRow6;
		int							uiLargeW;
		int							uiLargeH;
		int							uiSmallH;
		int							uiLargePreviewW;
		int							uiLargePreviewH;
		int							uiPreviewH;
		std::string					getDefaultVextexShaderString() { return mDefaultVextexShaderString; };
		std::string					getDefaultFragmentShaderString() { return mDefaultFragmentShaderString; };
		std::string					getMixFragmentShaderString() { return mMixFragmentShaderString; };
		std::string					getHydraFragmentShaderString() { return mHydraFragmentShaderString; };
		std::string					getMixetteFragmentShaderString() { return mMixetteFragmentShaderString; };
		std::string					getPostFragmentShaderString() { return mPostFragmentShaderString; };
	private:
		std::string					settingsFileName = "VD";
		//! default vertex shader
		std::string					mDefaultVextexShaderString;
		std::string					mDefaultFragmentShaderString;
		std::string					mMixFragmentShaderString; 
		std::string					mHydraFragmentShaderString;
		std::string					mMixetteFragmentShaderString;
		std::string					mPostFragmentShaderString;

	};

}
