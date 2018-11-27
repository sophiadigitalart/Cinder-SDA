#include "SDAFbo.h"

#include "cinder/gl/Texture.h"
#include "cinder/Xml.h"

using namespace ci;
using namespace ci::app;

namespace SophiaDigitalArt {
	SDAFbo::SDAFbo(SDASettingsRef aSDASettings, SDAAnimationRef aSDAAnimation)
		: mFilePathOrText("")
		, mFboName("fbo")
	{
		CI_LOG_V("SDAFbo constructor");
		mSDASettings = aSDASettings;
		mSDAAnimation = aSDAAnimation;
		mType = UNKNOWN;

		mInputTextureIndex = 0; 


		mPosX = mPosY = 0.0f;
		mZoom = 1.0f;
		isReady = false;
		mFlipV = mFlipH = false;

		// init the fbo whatever happens next
		fboFmt.setColorTextureFormat(fmt);
		mFbo = gl::Fbo::create(mSDASettings->mFboWidth, mSDASettings->mFboHeight, fboFmt);
		mThumbFbo = gl::Fbo::create(mSDASettings->mPreviewWidth, mSDASettings->mPreviewHeight, fboFmt);
		mError = "";
		// init with passthru shader
		//mShaderName = "0";
		mFboName = "default";

		// load feedback fragment shader
		try {
			mFboTextureShader = gl::GlslProg::create(mSDASettings->getDefaultVextexShaderString(), mSDASettings->getDefaultFragmentShaderString());
			CI_LOG_V("fbo default vtx-frag compiled");
		}
		catch (gl::GlslProgCompileExc &exc) {
			mError = string(exc.what());
			CI_LOG_V("fbo unable to load/compile vtx-frag shader:" + string(exc.what()));
		}
		catch (const std::exception &e) {
			mError = string(e.what());
			CI_LOG_V("fbo unable to load vtx-frag shader:" + string(e.what()));
		}
		if (mError.length() > 0) mSDASettings->mMsg = mError;
	}
	SDAFbo::~SDAFbo(void) {
	}

	XmlTree	SDAFbo::toXml() const {
		XmlTree		xml;
		xml.setTag("details");
		xml.setAttribute("path", mFilePathOrText);
		xml.setAttribute("width", mSDASettings->mFboWidth);
		xml.setAttribute("height", mSDASettings->mFboHeight);
		xml.setAttribute("shadername", mFboName);
		xml.setAttribute("inputtextureindex", mInputTextureIndex);
		return xml;
	}

	bool SDAFbo::fromXml(const XmlTree &xml) {
		mId = xml.getAttributeValue<string>("id", "");
		string mGlslPath = xml.getAttributeValue<string>("shadername", "0.frag");
		mWidth = xml.getAttributeValue<int>("width", mSDASettings->mFboWidth);
		mHeight = xml.getAttributeValue<int>("height", mSDASettings->mFboHeight);
		mInputTextureIndex = xml.getAttributeValue<int>("inputtextureindex", 0);
		CI_LOG_V("fbo id " + mId + "fbo shadername " + mGlslPath);
		mFboName = mGlslPath;
		// 20161209 problem on Mac mFboTextureShader->setLabel(mShaderName);
		return true;
	}
	void SDAFbo::setFragmentShader(unsigned int aShaderIndex, string aFragmentShaderString, string aName) {
		try {
			mFboTextureShader = gl::GlslProg::create(mSDASettings->getDefaultVextexShaderString(), aFragmentShaderString);
			mFboTextureFragmentShaderString = aFragmentShaderString; // set only if compiles successfully
			// 20161209 problem on Mac mFboTextureShader->setLabel(aName);
			mFboName = aName;
			mShaderIndex = aShaderIndex;
		}
		catch (gl::GlslProgCompileExc &exc) {
			mError = string(exc.what());
			CI_LOG_V("unable to load/compile fragment shader:" + string(exc.what()));
		}
		catch (const std::exception &e) {
			mError = string(e.what());
			CI_LOG_V("unable to load fragment shader:" + string(e.what()));
		}
	}
	void SDAFbo::setShaderIndex(unsigned int aShaderIndex) {
		mShaderIndex = aShaderIndex;
	}
	void SDAFbo::setPosition(int x, int y) {
		mPosX = ((float)x / (float)mSDASettings->mFboWidth) - 0.5;
		mPosY = ((float)y / (float)mSDASettings->mFboHeight) - 0.5;
	}
	void SDAFbo::setZoom(float aZoom) {
		mZoom = aZoom;
	}

	ci::ivec2 SDAFbo::getSize() {
		return mFbo->getSize();
	}

	ci::Area SDAFbo::getBounds() {
		return mFbo->getBounds();
	}

	GLuint SDAFbo::getId() {
		return mFbo->getId();
	}

	//std::string SDAFbo::getName() {
	//	return mShaderName + " fb:" + mId;
	//	//return mShaderName + " " + mId;
	//}
	//std::string SDAFbo::getShaderName() {
	//	return mShaderName;
	//}
	void SDAFbo::setInputTexture(SDATextureList aTextureList, unsigned int aTextureIndex) {
		mTextureList = aTextureList;
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mInputTextureIndex = aTextureIndex;
	}
	gl::GlslProgRef SDAFbo::getShader() {
		auto &uniforms = mFboTextureShader->getActiveUniforms();
		for (const auto &uniform : uniforms) {
			//CI_LOG_V(mFboTextureShader->getLabel() + ", getShader uniform name:" + uniform.getName());
			if (mSDAAnimation->isExistingUniform(uniform.getName())) {
				int uniformType = mSDAAnimation->getUniformType(uniform.getName());
				switch (uniformType)
				{
				case 0:
					// float
					mFboTextureShader->uniform(uniform.getName(), mSDAAnimation->getFloatUniformValueByName(uniform.getName()));
					break;
				case 1:
					// sampler2D
					mFboTextureShader->uniform(uniform.getName(), mInputTextureIndex);
					break;
				case 2:
					// vec2
					mFboTextureShader->uniform(uniform.getName(), mSDAAnimation->getVec2UniformValueByName(uniform.getName()));
					break;
				case 3:
					// vec3
					mFboTextureShader->uniform(uniform.getName(), mSDAAnimation->getVec3UniformValueByName(uniform.getName()));
					break;
				case 4:
					// vec4
					mFboTextureShader->uniform(uniform.getName(), mSDAAnimation->getVec4UniformValueByName(uniform.getName()));
					break;
				case 5:
					// int
					mFboTextureShader->uniform(uniform.getName(), mSDAAnimation->getIntUniformValueByName(uniform.getName()));
					break;
				case 6:
					// bool
					mFboTextureShader->uniform(uniform.getName(), mSDAAnimation->getBoolUniformValueByName(uniform.getName()));
					break;
				default:
					break;
				}
			}
			else {
				if (uniform.getName() != "ciModelViewProjection") {
					mSDASettings->mMsg = mFboName + ", uniform not found:" + uniform.getName();
					CI_LOG_V(mSDASettings->mMsg);
				}
			}
		}
		// feedback
		/*auto &fbuniforms = mFeedbackShader->getActiveUniforms();
		for (const auto &uniform : fbuniforms) {
			//CI_LOG_V(mFboTextureShader->getLabel() + ", getShader uniform name:" + uniform.getName());
			if (mSDAAnimation->isExistingUniform(uniform.getName())) {
				int uniformType = mSDAAnimation->getUniformType(uniform.getName());
				switch (uniformType)
				{
				case 0:
					// float
					mFeedbackShader->uniform(uniform.getName(), mSDAAnimation->getFloatUniformValueByName(uniform.getName()));
					break;
				case 1:
					// sampler2D
					mFeedbackShader->uniform(uniform.getName(), mCurrentFeedbackIndex);
					break;
				case 2:
					// vec2
					mFeedbackShader->uniform(uniform.getName(), mSDAAnimation->getVec2UniformValueByName(uniform.getName()));
					break;
				case 3:
					// vec3
					mFeedbackShader->uniform(uniform.getName(), mSDAAnimation->getVec3UniformValueByName(uniform.getName()));
					break;
				case 4:
					// vec4
					mFeedbackShader->uniform(uniform.getName(), mSDAAnimation->getVec4UniformValueByName(uniform.getName()));
					break;
				case 5:
					// int
					mFeedbackShader->uniform(uniform.getName(), mSDAAnimation->getIntUniformValueByName(uniform.getName()));
					break;
				case 6:
					// bool
					mFeedbackShader->uniform(uniform.getName(), mSDAAnimation->getBoolUniformValueByName(uniform.getName()));
					break;
				default:
					break;
				}
			}
			else {
				if (uniform.getName() != "ciModelViewProjection") {
					mSDASettings->mMsg =  "feedback shader, uniform not found:" + uniform.getName();
					CI_LOG_V(mSDASettings->mMsg);
				}
			}
		}*/
		return mFboTextureShader;
	}
	ci::gl::Texture2dRef SDAFbo::getRenderedTexture() {
		if (!isReady) {
			// render once for init
			getFboTexture();
			updateThumbFile();
			isReady = true;
		}
		return mRenderedTexture;
	}

	ci::gl::Texture2dRef SDAFbo::getFboTexture() {
		// TODO move this:
		getShader();
		gl::ScopedFramebuffer fbScp(mFbo);
		gl::clear(Color::black());
		if (mInputTextureIndex > mTextureList.size() - 1) mInputTextureIndex = 0;
		mTextureList[mInputTextureIndex]->getTexture()->bind(0);

		gl::ScopedGlslProg glslScope(mFboTextureShader);
		gl::drawSolidRect(Rectf(0, 0, mSDASettings->mFboWidth, mSDASettings->mFboHeight));

		mRenderedTexture = mFbo->getColorTexture();
		return mRenderedTexture;
	}

	void SDAFbo::updateThumbFile() {
		getFboTexture();
		if (mRenderedTexture) {
			string filename = getName() + ".jpg";
			fs::path fr = getAssetPath("") / "thumbs" / "jpg" / filename;

			if (!fs::exists(fr)) {
				CI_LOG_V(fr.string() + " does not exist, creating");
				// TODO move this:
				getShader();
				ci::gl::Texture2dRef mThumbTexture;
				gl::ScopedFramebuffer fbScp(mThumbFbo);
				gl::clear(Color::black());
				gl::ScopedViewport scpVp(ivec2(0), mThumbFbo->getSize());
				if (mInputTextureIndex > mTextureList.size() - 1) mInputTextureIndex = 0;
				mTextureList[mInputTextureIndex]->getTexture()->bind(0);
				mFboTextureShader->uniform("iResolution", vec3(mSDASettings->mPreviewWidth, mSDASettings->mPreviewHeight, 1.0));

				gl::ScopedGlslProg glslScope(mFboTextureShader);
				gl::drawSolidRect(Rectf(0, 0, mSDASettings->mPreviewWidth, mSDASettings->mPreviewHeight));
				
				mThumbTexture = mThumbFbo->getColorTexture();
				Surface s8(mThumbTexture->createSource());
				writeImage(writeFile(fr), s8);				
			}
		}
	}

} // namespace SophiaDigitalArt

