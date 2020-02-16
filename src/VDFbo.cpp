#include "VDFbo.h"

#include "cinder/gl/Texture.h"
#include "cinder/Xml.h"

using namespace ci;
using namespace ci::app;

namespace videodromm {
	VDFbo::VDFbo(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation)
		: mFilePathOrText("")
		, mFboName("fbo")
	{
		CI_LOG_V("VDFbo constructor");
		mVDSettings = aVDSettings;
		mVDAnimation = aVDAnimation;
		mType = UNKNOWN;

		mInputTextureIndex = 0; 


		mPosX = mPosY = 0.0f;
		mZoom = 1.0f;
		isReady = false;
		mFlipV = mFlipH = false;

		// init the fbo whatever happens next
		fboFmt.setColorTextureFormat(fmt);
		mFbo = gl::Fbo::create(mVDSettings->mFboWidth, mVDSettings->mFboHeight, fboFmt);
		mThumbFbo = gl::Fbo::create(mVDSettings->mPreviewWidth, mVDSettings->mPreviewHeight, fboFmt);
		mError = "";
		// init with passthru shader
		//mShaderName = "0";
		mFboName = "default";

		// load feedback fragment shader
		try {
			mFboTextureShader = gl::GlslProg::create(mVDSettings->getDefaultVextexShaderString(), mVDSettings->getDefaultFragmentShaderString());
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
		if (mError.length() > 0) mVDSettings->mMsg = mError;
	}
	VDFbo::~VDFbo(void) {
	}

	XmlTree	VDFbo::toXml() const {
		XmlTree		xml;
		xml.setTag("details");
		xml.setAttribute("path", mFilePathOrText);
		xml.setAttribute("width", mVDSettings->mFboWidth);
		xml.setAttribute("height", mVDSettings->mFboHeight);
		xml.setAttribute("shadername", mFboName);
		xml.setAttribute("inputtextureindex", mInputTextureIndex);
		return xml;
	}

	bool VDFbo::fromXml(const XmlTree &xml) {
		mId = xml.getAttributeValue<string>("id", "");
		string mGlslPath = xml.getAttributeValue<string>("shadername", "0.frag");
		mWidth = xml.getAttributeValue<int>("width", mVDSettings->mFboWidth);
		mHeight = xml.getAttributeValue<int>("height", mVDSettings->mFboHeight);
		// 20200216 bug fix? mInputTextureIndex = xml.getAttributeValue<int>("inputtextureindex", 0);
		CI_LOG_V("fbo id " + mId + "fbo shadername " + mGlslPath);
		mFboName = mGlslPath;
		// 20161209 problem on Mac mFboTextureShader->setLabel(mShaderName);
		return true;
	}
	void VDFbo::setFragmentShader(unsigned int aShaderIndex, string aFragmentShaderString, string aName) {
		try {
			mFboTextureShader = gl::GlslProg::create(mVDSettings->getDefaultVextexShaderString(), aFragmentShaderString);
			mFboTextureFragmentShaderString = aFragmentShaderString; // set only if compiles successfully
			// 20161209 problem on Mac mFboTextureShader->setLabel(aName);
			mFboName = aName;
			mShaderIndex = aShaderIndex;
		}
		catch (gl::GlslProgCompileExc &exc) {
			mError = string(exc.what());
			CI_LOG_V("fbo, unable to load/compile fragment shader:" + string(exc.what()));
		}
		catch (const std::exception &e) {
			mError = string(e.what());
			CI_LOG_V("fbo, unable to load fragment shader:" + string(e.what()));
		}
		mVDSettings->mNewMsg = true;
		mVDSettings->mMsg = mError;
	}
	void VDFbo::setShaderIndex(unsigned int aShaderIndex) {
		mShaderIndex = aShaderIndex;
	}
	void VDFbo::setPosition(int x, int y) {
		mPosX = ((float)x / (float)mVDSettings->mFboWidth) - 0.5;
		mPosY = ((float)y / (float)mVDSettings->mFboHeight) - 0.5;
	}
	void VDFbo::setZoom(float aZoom) {
		mZoom = aZoom;
	}

	ci::ivec2 VDFbo::getSize() {
		return mFbo->getSize();
	}

	ci::Area VDFbo::getBounds() {
		return mFbo->getBounds();
	}

	GLuint VDFbo::getId() {
		return mFbo->getId();
	}

	//std::string VDFbo::getName() {
	//	return mShaderName + " fb:" + mId;
	//	//return mShaderName + " " + mId;
	//}
	//std::string VDFbo::getShaderName() {
	//	return mShaderName;
	//}
	void VDFbo::setInputTexture(VDTextureList aTextureList, unsigned int aTextureIndex) {
		mTextureList = aTextureList;
		if (aTextureIndex > mTextureList.size() - 1) aTextureIndex = mTextureList.size() - 1;
		mInputTextureIndex = aTextureIndex;
	}
	/*
	precision mediump float;
	uniform float frequency194;
	uniform float sync195;
	uniform float offset196;
	uniform float r197;
	uniform float g198;
	uniform float b199;
	uniform float frequency200;
	uniform float sync201;
	uniform float offset202;
	uniform float amount204;
	uniform sampler2D tex206;
	uniform float multiple207;
	uniform float offset208;
	uniform float angle209;
	uniform float speed210;
	uniform float time;
	uniform vec2 resolution;
	varying vec2 uv;
	//\tSimplex 3D Noise\n    //\tby Ian McEwan, Ashima Arts\n    
	vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
	vec4 taylorInvSqrt(vec4 r){return 1.79284291400159 - 0.85373472095314 * r;}
	float _noise(vec3 v){
	const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
	const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);
	// First corner
	vec3 i  = floor(v + dot(v, C.yyy) );
	
	*/
	gl::GlslProgRef VDFbo::getShader() {
		int index = 300;
		int texIndex = 0;
		string name;
		auto &uniforms = mFboTextureShader->getActiveUniforms();
		for (const auto &uniform : uniforms) {
			name = uniform.getName();
			//CI_LOG_V(mFboTextureShader->getLabel() + ", getShader uniform name:" + uniform.getName());
			if (mVDAnimation->isExistingUniform(name)) {
				int uniformType = mVDAnimation->getUniformType(name);
				switch (uniformType)
				{
				case 0:
					// float
					mFboTextureShader->uniform(name, mVDAnimation->getFloatUniformValueByName(name));
					break;
				case 1:
					// sampler2D
					mFboTextureShader->uniform(name,  mInputTextureIndex);// cinder::gl::GlslProg::logUniformWrongType[1021] Uniform type mismatch for "iChannel0", expected SAMPLER_2D and received uint32_t
					break;
				case 2:
					// vec2
					mFboTextureShader->uniform(name, mVDAnimation->getVec2UniformValueByName(name));
					break;
				case 3:
					// vec3
					mFboTextureShader->uniform(name, mVDAnimation->getVec3UniformValueByName(name));
					break;
				case 4:
					// vec4
					mFboTextureShader->uniform(name, mVDAnimation->getVec4UniformValueByName(name));
					break;
				case 5:
					// int
					mFboTextureShader->uniform(name, mVDAnimation->getIntUniformValueByName(name));
					break;
				case 6:
					// bool
					mFboTextureShader->uniform(name, mVDAnimation->getBoolUniformValueByName(name));
					break;
				default:
					break;
				}
			}
			else {
				if (name != "ciModelViewProjection") {
					mVDSettings->mMsg = mFboName + ", uniform not found:" + name + " type:" + toString( uniform.getType());
					CI_LOG_V(mVDSettings->mMsg); 
					int firstDigit = name.find_first_of("0123456789");
					// if valid image sequence (contains a digit)
					if (firstDigit > -1) {
						index = std::stoi(name.substr(firstDigit));
					}
					switch (uniform.getType())
					{
					case 5126:
						mVDAnimation->createFloatUniform(name, 400 + index, 0.31f, 0.0f, 1000.0f);
						mFboTextureShader->uniform(name, mVDAnimation->getFloatUniformValueByName(name));
						break;
					case 35664:
						//mVDAnimation->createvec2(uniform.getName(), 310 + , 0);
						//++;	
						break;
					case 35678:
						mVDAnimation->createSampler2DUniform(uniform.getName(), 310 + texIndex, 0);
						texIndex++;
						break;
					default:
						break;
					}
				}
			}
		}
		return mFboTextureShader;
	}
	ci::gl::Texture2dRef VDFbo::getRenderedTexture() {
		if (!isReady) {
			// render once for init
			getFboTexture();
			updateThumbFile();
			isReady = true;
		}
		return mRenderedTexture;
	}

	ci::gl::Texture2dRef VDFbo::getFboTexture() {
		// TODO move this:
		getShader();
		gl::ScopedFramebuffer fbScp(mFbo);
		gl::clear(Color::black());

		if (mInputTextureIndex > mTextureList.size() - 1) mInputTextureIndex = 0;
		mTextureList[mInputTextureIndex]->getTexture()->bind(0);

		gl::ScopedGlslProg glslScope(mFboTextureShader);
		// TODO: test gl::ScopedViewport sVp(0, 0, mFbo->getWidth(), mFbo->getHeight());

		//gl::drawSolidRect(Rectf(0, 0, mVDSettings->mPreviewWidth, mVDSettings->mPreviewHeight));
		gl::drawSolidRect(Rectf(0, 0, mVDSettings->mFboWidth, mVDSettings->mFboHeight));
		mRenderedTexture = mFbo->getColorTexture();
		return mRenderedTexture;
	}

	void VDFbo::updateThumbFile() {
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
				
				mFboTextureShader->uniform("iBlendmode", mVDSettings->iBlendmode);
				mFboTextureShader->uniform("iTime", mVDAnimation->getFloatUniformValueByIndex(0));
				// was vec3(mVDSettings->mFboWidth, mVDSettings->mFboHeight, 1.0)):
				mFboTextureShader->uniform("iResolution", vec3(mThumbFbo->getWidth(), mThumbFbo->getHeight(), 1.0));
				//mFboTextureShader->uniform("iChannelResolution", mVDSettings->iChannelResolution, 4);
				// 20180318 mFboTextureShader->uniform("iMouse", mVDAnimation->getVec4UniformValueByName("iMouse"));
				mFboTextureShader->uniform("iMouse", vec3(mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IMOUSEX), mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IMOUSEY), mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IMOUSEZ)));
				mFboTextureShader->uniform("iDate", mVDAnimation->getVec4UniformValueByName("iDate"));
				mFboTextureShader->uniform("iWeight0", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IWEIGHT0));	// weight of channel 0
				mFboTextureShader->uniform("iWeight1", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IWEIGHT1));	// weight of channel 1
				mFboTextureShader->uniform("iWeight2", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IWEIGHT2));	// weight of channel 2
				mFboTextureShader->uniform("iWeight3", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IWEIGHT3)); // texture
				mFboTextureShader->uniform("iWeight4", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IWEIGHT4)); // texture
				mFboTextureShader->uniform("iChannel0", 0); // fbo shader 
				mFboTextureShader->uniform("iChannel1", 1); // fbo shader
				mFboTextureShader->uniform("iChannel2", 2); // texture 1
				mFboTextureShader->uniform("iChannel3", 3); // texture 2
				mFboTextureShader->uniform("iChannel4", 4); // texture 3

				mFboTextureShader->uniform("iRatio", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IRATIO));//check if needed: +1;
				mFboTextureShader->uniform("iRenderXY", mVDSettings->mRenderXY);
				mFboTextureShader->uniform("iZoom", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IZOOM));
				mFboTextureShader->uniform("iAlpha", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IFA) * mVDSettings->iAlpha);
				mFboTextureShader->uniform("iChromatic", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->ICHROMATIC));
				mFboTextureShader->uniform("iRotationSpeed", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IROTATIONSPEED));
				mFboTextureShader->uniform("iCrossfade", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IXFADE));
				mFboTextureShader->uniform("iPixelate", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IPIXELATE));
				mFboTextureShader->uniform("iExposure", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IEXPOSURE));
				mFboTextureShader->uniform("iToggle", (int)mVDAnimation->getBoolUniformValueByIndex(mVDSettings->ITOGGLE));
				mFboTextureShader->uniform("iGreyScale", (int)mVDSettings->iGreyScale);
				mFboTextureShader->uniform("iBackgroundColor", mVDAnimation->getVec3UniformValueByName("iBackgroundColor"));
				mFboTextureShader->uniform("iVignette", (int)mVDAnimation->getBoolUniformValueByIndex(mVDSettings->IVIGN));
				mFboTextureShader->uniform("iInvert", (int)mVDAnimation->getBoolUniformValueByIndex(mVDSettings->IINVERT));
				mFboTextureShader->uniform("iTempoTime", mVDAnimation->getFloatUniformValueByName("iTempoTime"));
				mFboTextureShader->uniform("iGlitch", (int)mVDAnimation->getBoolUniformValueByIndex(mVDSettings->IGLITCH));
				mFboTextureShader->uniform("iTrixels", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->ITRIXELS));
				mFboTextureShader->uniform("iRedMultiplier", mVDAnimation->getFloatUniformValueByName("iRedMultiplier"));
				mFboTextureShader->uniform("iGreenMultiplier", mVDAnimation->getFloatUniformValueByName("iGreenMultiplier"));
				mFboTextureShader->uniform("iBlueMultiplier", mVDAnimation->getFloatUniformValueByName("iBlueMultiplier"));
				mFboTextureShader->uniform("iFlipH", (int)mVDAnimation->getBoolUniformValueByIndex(mVDSettings->IFLIPH));
				mFboTextureShader->uniform("iFlipV", (int)mVDAnimation->getBoolUniformValueByIndex(mVDSettings->IFLIPV));
				mFboTextureShader->uniform("pixelX", mVDAnimation->getFloatUniformValueByName("pixelX"));
				mFboTextureShader->uniform("pixelY", mVDAnimation->getFloatUniformValueByName("pixelY"));
				mFboTextureShader->uniform("iXorY", mVDSettings->iXorY);
				mFboTextureShader->uniform("iBadTv", mVDAnimation->getFloatUniformValueByName("iBadTv"));
				mFboTextureShader->uniform("iFps", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IFPS));
				mFboTextureShader->uniform("iContour", mVDAnimation->getFloatUniformValueByName("iContour"));
				mFboTextureShader->uniform("iSobel", mVDAnimation->getFloatUniformValueByName("iSobel"));

				gl::ScopedGlslProg glslScope(mFboTextureShader);
				gl::drawSolidRect(Rectf(0, 0, mThumbFbo->getWidth(), mThumbFbo->getHeight()));
				
				mThumbTexture = mThumbFbo->getColorTexture();
				Surface s8(mThumbTexture->createSource());
				writeImage(writeFile(fr), s8);				
			}
		}
	}

} // namespace videodromm

