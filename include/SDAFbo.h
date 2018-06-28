#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Xml.h"
#include "cinder/Json.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "cinder/Timeline.h"

// Settings
#include "SDASettings.h"
// Animation
#include "SDAAnimation.h"
// textures
#include "SDATexture.h"
// shaders
#include "SDAShader.h"

#include <atomic>
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace SophiaDigitalArt;

namespace SophiaDigitalArt
{
	// stores the pointer to the SDAFbo instance
	typedef std::shared_ptr<class SDAFbo> 			SDAFboRef;
	typedef std::vector<SDAFboRef>					SDAFboList;

	class SDAFbo : public SDATexture{
	public:
		SDAFbo(SDASettingsRef aSDASettings, SDAAnimationRef aSDAAnimation);
		~SDAFbo(void);
		static SDAFboRef create(SDASettingsRef aSDASettings, SDAAnimationRef aSDAAnimation) {
			return std::make_shared<SDAFbo>(aSDASettings, aSDAAnimation);
		}
		//! returns a shared pointer to this fbo
		SDAFboRef						getPtr() { return std::static_pointer_cast<SDAFbo>(shared_from_this()); }
		ci::ivec2						getSize();
		ci::Area						getBounds();
		GLuint							getId();
		//! returns the type
		TextureType						getType() { return mType; };
		std::string						getName();
		std::string						getShaderName();
		void							setLabel(string aLabel) { mFboTextureShader->setLabel(aLabel); };
		bool							isFlipH() { return mFlipH; };
		bool							isFlipV() { return mFlipV; };
		void							flipV() { mFlipV = !mFlipV; };
		//!
		bool							fromXml(const ci::XmlTree &xml);
		//!
		XmlTree							toXml() const;
		// move, rotate, zoom methods
		void							setPosition(int x, int y);
		void							setZoom(float aZoom);
		// shader
		void							setShaderIndex(unsigned int aShaderIndex);
		unsigned int					getShaderIndex() { return mShaderIndex; };
		void							setFragmentShader(unsigned int aShaderIndex, string aFragmentShaderString, string aName);
		// textures
		void							setInputTexture(SDATextureList aTextureList, unsigned int aTextureIndex = 0);
		unsigned int					getInputTextureIndex() { return mInputTextureIndex; };
		ci::gl::Texture2dRef			getFboTexture();
		void							updateThumbFile();
		gl::GlslProgRef					getShader();
		ci::gl::Texture2dRef			getRenderedTexture();

	protected:
		std::string						mFboName;
		bool							mFlipV;
		bool							mFlipH;
		TextureType						mType;
		std::string						mFilePathOrText;
		//bool							mTopDown;
		float							mPosX;
		float							mPosY;
		float							mZoom;
		//! default fragment shader
		std::string						mFboTextureFragmentShaderString;
		//! shader
		gl::GlslProgRef					mFboTextureShader;

		string							mError;
	private:
		// Settings
		SDASettingsRef					mSDASettings;
		// Animation
		SDAAnimationRef					mSDAAnimation;

		//! Fbo
		gl::FboRef						mFbo;
		gl::Texture::Format				fmt;
		gl::Fbo::Format					fboFmt;
		//! Input textures
		//map<int, ci::gl::Texture2dRef>	mInputTextures;
		SDATextureList					mTextureList;
		unsigned int					mInputTextureIndex;
		//! Shaders
		string							mShaderName;
		unsigned int					mShaderIndex;
		string							mId;
		// Output texture
		ci::gl::Texture2dRef			mRenderedTexture;
		bool							isReady;
	};
}
