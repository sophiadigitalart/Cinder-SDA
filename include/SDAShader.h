#pragma once

#include "cinder/Cinder.h"
#include "cinder/app/App.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Utilities.h"
#include "cinder/Timeline.h"

#include "Resources.h"
// Logger
#include "SDALog.h"
// Settings
#include "SDASettings.h"
// Animation
#include "SDAAnimation.h"
// Watchdog
#include "Watchdog.h"
// std regex
#include <regex>

#pragma warning(push)
#pragma warning(disable: 4996) // _CRT_SECURE_NO_WARNINGS

using namespace ci;
using namespace ci::app;
using namespace std;

namespace SophiaDigitalArt
{
	// stores the pointer to the SDAShader instance
	typedef std::shared_ptr<class SDAShader>	SDAShaderRef;
	typedef std::vector<SDAShaderRef>			SDAShaderList;


	class SDAShader {
	public:
		SDAShader(SDASettingsRef aSDASettings, SDAAnimationRef aSDAAnimation, string aFragmentShaderFilePath, string aFragmentShaderString = "");
		//void update();
		static SDAShaderRef	create(SDASettingsRef aSDASettings, SDAAnimationRef aSDAAnimation, string aFragmentShaderFilePath, string aFragmentShaderString = "")
		{
			return shared_ptr<SDAShader>(new SDAShader(aSDASettings, aSDAAnimation, aFragmentShaderFilePath, aFragmentShaderString));
		}
		//void fromXml(const XmlTree &xml);
		gl::GlslProgRef					getShader();
		string							getName();
		bool							loadFragmentStringFromFile(string aFileName);
		string							getFragmentString() {
			if (mFragmentShaderString.empty()) mFragmentShaderString = "void main(void){vec2 uv = gl_FragCoord.xy / iResolution.xy;fragColor = texture(iChannel0, uv);}";
			if (mFragmentShaderString.size() < 1 || mFragmentShaderString.size() > 256000) mFragmentShaderString = "void main(void){vec2 uv = gl_FragCoord.xy / iResolution.xy;fragColor = texture(iChannel0, uv);}";
			return mFragmentShaderString;
		};
		bool							setFragmentString(string aFragmentShaderString, string aName = "");
		// thumb image
		//ci::gl::Texture2dRef			getThumb();
		bool							isValid() { return mValid; };
		bool							isActive() { return mActive; };
		void							setActive(bool active) { mActive = active; };
		void							removeShader();
	private:
		// Settings
		SDASettingsRef					mSDASettings;
		// Animation
		SDAAnimationRef					mSDAAnimation;

		string							mId;
		gl::GlslProgRef					mShader;
		string							mName;
		string							mText;
		bool							mActive;
		int								mMicroSeconds;
		string							mError;
        bool							mValid;
		//! fragment shader
		std::string						mFragmentShaderString;
		std::string						mFragmentShaderFilePath;
		fs::path						mFragFile;
		// include shader lines
		std::string						shaderInclude;
		// fbo
		//gl::FboRef						mThumbFbo;
		//ci::gl::Texture2dRef			mThumbTexture;
	};
}
