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
		// TODO: test gl::ScopedViewport sVp(0, 0, mFbo->getWidth(), mFbo->getHeight());

		gl::drawSolidRect(Rectf(0, 0, mSDASettings->mPreviewWidth, mSDASettings->mPreviewHeight));

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

				gl::ScopedFramebuffer fbo(mThumbFbo);
				gl::ScopedViewport viewport(0, 0, mThumbFbo->getWidth(), mThumbFbo->getHeight());
				gl::ScopedMatrices matrices;
				gl::setMatricesWindow(mThumbFbo->getSize(), false);


				ci::gl::Texture2dRef mThumbTexture;
				//gl::ScopedFramebuffer fbScp(mThumbFbo);
				gl::clear(Color::black());
				//gl::ScopedViewport scpVp(ivec2(0), mThumbFbo->getSize());
				if (mInputTextureIndex > mTextureList.size() - 1) mInputTextureIndex = 0;
				mTextureList[mInputTextureIndex]->getTexture()->bind(0);
				
				mFboTextureShader->uniform("iBlendmode", mSDASettings->iBlendmode);
				mFboTextureShader->uniform("iTime", mSDAAnimation->getFloatUniformValueByIndex(0));
				// was vec3(mSDASettings->mFboWidth, mSDASettings->mFboHeight, 1.0)):
				mFboTextureShader->uniform("iResolution", vec3(mThumbFbo->getWidth(), mThumbFbo->getHeight(), 1.0));
				//mFboTextureShader->uniform("iChannelResolution", mSDASettings->iChannelResolution, 4);
				// 20180318 mFboTextureShader->uniform("iMouse", mSDAAnimation->getVec4UniformValueByName("iMouse"));
				mFboTextureShader->uniform("iMouse", vec3(mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IMOUSEX), mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IMOUSEY), mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IMOUSEZ)));
				mFboTextureShader->uniform("iDate", mSDAAnimation->getVec4UniformValueByName("iDate"));
				mFboTextureShader->uniform("iWeight0", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT0));	// weight of channel 0
				mFboTextureShader->uniform("iWeight1", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT1));	// weight of channel 1
				mFboTextureShader->uniform("iWeight2", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT2));	// weight of channel 2
				mFboTextureShader->uniform("iWeight3", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT3)); // texture
				mFboTextureShader->uniform("iWeight4", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IWEIGHT4)); // texture
				mFboTextureShader->uniform("iChannel0", 0); // fbo shader 
				mFboTextureShader->uniform("iChannel1", 1); // fbo shader
				mFboTextureShader->uniform("iChannel2", 2); // texture 1
				mFboTextureShader->uniform("iChannel3", 3); // texture 2
				mFboTextureShader->uniform("iChannel4", 4); // texture 3

				mFboTextureShader->uniform("iRatio", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IRATIO));//check if needed: +1;
				mFboTextureShader->uniform("iRenderXY", mSDASettings->mRenderXY);
				mFboTextureShader->uniform("iZoom", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IZOOM));
				mFboTextureShader->uniform("iAlpha", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IFA) * mSDASettings->iAlpha);
				mFboTextureShader->uniform("iChromatic", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->ICHROMATIC));
				mFboTextureShader->uniform("iRotationSpeed", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IROTATIONSPEED));
				mFboTextureShader->uniform("iCrossfade", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IXFADE));
				mFboTextureShader->uniform("iPixelate", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IPIXELATE));
				mFboTextureShader->uniform("iExposure", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IEXPOSURE));
				mFboTextureShader->uniform("iToggle", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->ITOGGLE));
				mFboTextureShader->uniform("iGreyScale", (int)mSDASettings->iGreyScale);
				mFboTextureShader->uniform("iBackgroundColor", mSDAAnimation->getVec3UniformValueByName("iBackgroundColor"));
				mFboTextureShader->uniform("iVignette", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IVIGN));
				mFboTextureShader->uniform("iInvert", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IINVERT));
				mFboTextureShader->uniform("iTempoTime", mSDAAnimation->getFloatUniformValueByName("iTempoTime"));
				mFboTextureShader->uniform("iGlitch", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IGLITCH));
				mFboTextureShader->uniform("iTrixels", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->ITRIXELS));
				mFboTextureShader->uniform("iRedMultiplier", mSDAAnimation->getFloatUniformValueByName("iRedMultiplier"));
				mFboTextureShader->uniform("iGreenMultiplier", mSDAAnimation->getFloatUniformValueByName("iGreenMultiplier"));
				mFboTextureShader->uniform("iBlueMultiplier", mSDAAnimation->getFloatUniformValueByName("iBlueMultiplier"));
				mFboTextureShader->uniform("iFlipH", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IFLIPH));
				mFboTextureShader->uniform("iFlipV", (int)mSDAAnimation->getBoolUniformValueByIndex(mSDASettings->IFLIPV));
				mFboTextureShader->uniform("iParam1", mSDASettings->iParam1);
				mFboTextureShader->uniform("iParam2", mSDASettings->iParam2);
				mFboTextureShader->uniform("iXorY", mSDASettings->iXorY);
				mFboTextureShader->uniform("iBadTv", mSDAAnimation->getFloatUniformValueByName("iBadTv"));
				mFboTextureShader->uniform("iFps", mSDAAnimation->getFloatUniformValueByIndex(mSDASettings->IFPS));
				mFboTextureShader->uniform("iContour", mSDAAnimation->getFloatUniformValueByName("iContour"));
				mFboTextureShader->uniform("iSobel", mSDAAnimation->getFloatUniformValueByName("iSobel"));

				gl::ScopedGlslProg glslScope(mFboTextureShader);
				gl::drawSolidRect(Rectf(0, 0, mThumbFbo->getWidth(), mThumbFbo->getHeight()));
				
				mThumbTexture = mThumbFbo->getColorTexture();
				Surface s8(mThumbTexture->createSource());
				writeImage(writeFile(fr), s8);				
			}
		}
	}

} // namespace SophiaDigitalArt

