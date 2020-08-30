#pragma once

#include "cinder/Cinder.h"
#include "cinder/app/App.h"
//!  json
#include "cinder/Json.h"
//!  Settings
#include "VDSettings.h"

using namespace ci;
using namespace ci::app;

namespace videodromm
{

	//enum class UniformTypes { FLOAT, SAMPLER2D, VEC2, VEC3, VEC4, INT, BOOL };
	// cinder::gl::GlslProg::Uniform
	struct VDUniformStruct
	{
		std::string						name;
		int								uniformType;
		int								index;
		float							defaultValue;
		float							floatValue;
		float							minValue;
		float							maxValue;
		int								anim;
		bool							autotime;
		bool							automatic;
		bool							autobass;
		bool							automid;
		bool							autotreble;
		int								textureIndex;
		//bool							isValid;
	};

	// stores the pointer to the VDUniform instance
	typedef std::shared_ptr<class VDUniform> VDUniformRef;

	class VDUniform {
	public:
		VDUniform(VDSettingsRef aVDSettings);

		static VDUniformRef				create(VDSettingsRef aVDSettings)
		{
			return std::shared_ptr<VDUniform>(new VDUniform(aVDSettings));
		}
		int								getUniformType(unsigned int aIndex) {
			return shaderUniforms[aIndex].uniformType;
		}
		int								getUniformTypeByName(const std::string& aName) {
			return shaderUniforms[stringToIndex(aName)].uniformType;
		}
		std::string						getUniformName(unsigned int aIndex) {
			return shaderUniforms[aIndex].name;
		}
		float							getUniformDefaultValue(unsigned int aIndex) {
			return shaderUniforms[aIndex].defaultValue;
		}
		float							getUniformMinValue(unsigned int aIndex) {
			return shaderUniforms[aIndex].minValue;
		}
		float							getUniformMaxValue(unsigned int aIndex) {
			return shaderUniforms[aIndex].maxValue;
		}
		int								getUniformTextureIndex(unsigned int aIndex) {
			return shaderUniforms[aIndex].textureIndex;
		}
		void							setAnim(unsigned int aCtrl, unsigned int aAnim) {
			shaderUniforms[aCtrl].anim = aAnim;
		}
		bool setUniformValue(unsigned int aIndex, float aValue) {
			bool rtn = false;
			// we can't change TIME at index 0
			if (aIndex > 0) {
				/*if (aIndex == 31) {
					CI_LOG_V("old value " + toString(shaderUniforms[getUniformNameForIndex(aIndex)].floatValue) + " newvalue " + toString(aValue));
				}*/
				//string uniformName = getUniformNameForIndex(aIndex);
				if (shaderUniforms[aIndex].floatValue != aValue) {
					if ((aValue >= shaderUniforms[aIndex].minValue && aValue <= shaderUniforms[aIndex].maxValue) || shaderUniforms[aIndex].anim > 0) {
						shaderUniforms[aIndex].floatValue = aValue;
						rtn = true;
					}
				}
				// not all controls are from 0.0 to 1.0
				/* not working float lerpValue = lerp<float, float>(shaderUniforms[getUniformNameForIndex(aIndex)].minValue, shaderUniforms[getUniformNameForIndex(aIndex)].maxValue, aValue);
				if (shaderUniforms[getUniformNameForIndex(aIndex)].floatValue != lerpValue) {
					shaderUniforms[getUniformNameForIndex(aIndex)].floatValue = lerpValue;
					rtn = true;
				}*/
			}
			else {
				// no max 
				shaderUniforms[aIndex].floatValue = aValue;
			}
			return rtn;
		}
		int								getUniformIndexForName(const std::string& aName) {
			return shaderUniforms[stringToIndex(aName)].index;
		};
		bool							setBoolUniformValueByIndex(unsigned int aIndex, bool aValue) {
			shaderUniforms[aIndex].floatValue = aValue;
			return aValue;
		}
		void							setUniformValueByName(const std::string& aName, int aValue) {
			if (aName == "") {
				CI_LOG_E("empty error");
			}
			else {
				shaderUniforms[stringToIndex(aName)].floatValue = aValue;
			}
		}
		void							setUniformValue(unsigned int aIndex, int aValue) {

			if (mVDSettings->IBEAT == aIndex) {
				if (aValue != mLastBeat) {
					mLastBeat = aValue;
					if (aValue == 0) setUniformValue(mVDSettings->IBAR, getIntUniformValueByIndex(mVDSettings->IBAR) + 1);
				}
			}
			shaderUniforms[aIndex].floatValue = aValue;
		}
		void							setFloatUniformValueByName(const std::string& aName, float aValue) {
			if (aName == "") {
				CI_LOG_E("empty error");
			}
			else {
				shaderUniforms[stringToIndex(aName)].floatValue = aValue;
			}
		}
		void setVec2UniformValueByName(const std::string& aName, vec2 aValue) {
			if (aName == "") {
				CI_LOG_E("empty error");
			}
			else {
				shaderUniforms[stringToIndex(aName + "X")].floatValue = aValue.x;
				shaderUniforms[stringToIndex(aName + "Y")].floatValue = aValue.y;
			}
		}
		void setVec2UniformValueByIndex(unsigned int aIndex, vec2 aValue) {
			//shaderUniforms[aIndex].vec2Value = aValue; //IRES 120 IRESOLUTIONX 121 IRESOLUTIONY 122 TODO COLOR RENDERSIZE
			shaderUniforms[aIndex + 1].floatValue = aValue.x;
			shaderUniforms[aIndex + 2].floatValue = aValue.y;
		}
		void setVec3UniformValueByName(const std::string& aName, vec3 aValue) {
			if (aName == "") {
				CI_LOG_E("empty error");
			}
			else {
				//shaderUniforms[stringToIndex(aName)].vec3Value = aValue;
				shaderUniforms[stringToIndex(aName + "X")].floatValue = aValue.x;
				shaderUniforms[stringToIndex(aName + "Y")].floatValue = aValue.y;
				shaderUniforms[stringToIndex(aName + "Z")].floatValue = aValue.z;
			}
		}
		void setVec3UniformValueByIndex(unsigned int aIndex, vec3 aValue) {
			//shaderUniforms[aIndex].vec3Value = aValue;
			shaderUniforms[aIndex + 1].floatValue = aValue.x;
			shaderUniforms[aIndex + 2].floatValue = aValue.y;
			shaderUniforms[aIndex + 3].floatValue = aValue.z;
		}
		void setVec4UniformValueByName(const std::string& aName, vec4 aValue) {
			if (aName == "") {
				CI_LOG_E("empty error");
			}
			else {
				//shaderUniforms[stringToIndex(aName)].vec4Value = aValue;
				shaderUniforms[stringToIndex(aName + "X")].floatValue = aValue.x;
				shaderUniforms[stringToIndex(aName + "Y")].floatValue = aValue.y;
				shaderUniforms[stringToIndex(aName + "Z")].floatValue = aValue.z;
				shaderUniforms[stringToIndex(aName + "W")].floatValue = aValue.w;
			}
		}
		void setVec4UniformValueByIndex(unsigned int aIndex, vec4 aValue) {
			//shaderUniforms[aIndex].vec4Value = aValue;
			shaderUniforms[aIndex + 1].floatValue = aValue.x;
			shaderUniforms[aIndex + 2].floatValue = aValue.y;
			shaderUniforms[aIndex + 3].floatValue = aValue.z;
			shaderUniforms[aIndex + 4].floatValue = aValue.w;
		}
		vec2							getVec2UniformValueByName(const std::string& aName) {
			return vec2(shaderUniforms[stringToIndex(aName + "X")].floatValue,
				shaderUniforms[stringToIndex(aName + "Y")].floatValue);
			//return shaderUniforms[stringToIndex(aName)].vec2Value;
		}
		vec3							getVec3UniformValueByName(const std::string& aName) {
			return vec3(shaderUniforms[stringToIndex(aName + "X")].floatValue,
				shaderUniforms[stringToIndex(aName + "Y")].floatValue,
				shaderUniforms[stringToIndex(aName + "Z")].floatValue);
			//return shaderUniforms[stringToIndex(aName)].vec3Value;
			// OUI AU TEL
		}
		vec4							getVec4UniformValueByName(const std::string& aName) {
			return vec4(shaderUniforms[stringToIndex(aName + "X")].floatValue,
				shaderUniforms[stringToIndex(aName + "Y")].floatValue,
				shaderUniforms[stringToIndex(aName + "Z")].floatValue,
				shaderUniforms[stringToIndex(aName + "W")].floatValue);
			//return shaderUniforms[stringToIndex(aName)].vec4Value;
		}
		int								getIntUniformValueByName(const std::string& aName) {
			return shaderUniforms[stringToIndex(aName)].floatValue;
		};
		bool							getBoolUniformValueByIndex(unsigned int aIndex) {
			return shaderUniforms[aIndex].floatValue;
		}
		float							getMinUniformValue(unsigned int aIndex) {
			return shaderUniforms[aIndex].minValue;
		}
		float							getMaxUniformValue(unsigned int aIndex) {
			return shaderUniforms[aIndex].maxValue;
		}
		bool							getBoolUniformValueByName(const std::string& aName) {
			return shaderUniforms[stringToIndex(aName)].floatValue;
		}
		float							getMinUniformValueByName(const std::string& aName) {
			if (aName == "") {
				CI_LOG_V("empty error");
			}

			return shaderUniforms[stringToIndex(aName)].minValue;
		}
		float							getMaxUniformValueByName(const std::string& aName) {
			if (aName == "") {
				CI_LOG_V("empty error");
			}

			return shaderUniforms[stringToIndex(aName)].maxValue;
		}
		float							getUniformValue(unsigned int aIndex) {
			return shaderUniforms[aIndex].floatValue;
		}
		int								getUniformAnim(unsigned int aIndex) {
			return shaderUniforms[aIndex].anim;
		}
		float							getFloatUniformDefaultValueByIndex(unsigned int aIndex) {
			return shaderUniforms[aIndex].defaultValue;
		}
		int								getIntUniformValueByIndex(unsigned int aIndex) {
			return shaderUniforms[aIndex].floatValue;
		}
		int								getSampler2DUniformValueByName(const std::string& aName) {
			return shaderUniforms[stringToIndex(aName)].textureIndex;
		}
		float							getUniformValueByName(const std::string& aName) {
			if (aName == "") {
				CI_LOG_V("getUniformValueByName name empty");
				return 1.0f;
			}
			else {
				return shaderUniforms[stringToIndex(aName)].floatValue;
			}
		}
		// public for hydra
		void createFloatUniform(const std::string& aName, int aCtrlIndex, float aValue = 1.0f, float aMin = 0.0f, float aMax = 1.0f) {
			if (aName != "") {
				shaderUniforms[aCtrlIndex].name = aName;
				shaderUniforms[aCtrlIndex].minValue = aMin;
				shaderUniforms[aCtrlIndex].maxValue = aMax;
				shaderUniforms[aCtrlIndex].defaultValue = aValue;
				shaderUniforms[aCtrlIndex].anim = 0;
				shaderUniforms[aCtrlIndex].index = aCtrlIndex;
				shaderUniforms[aCtrlIndex].floatValue = aValue;
				shaderUniforms[aCtrlIndex].uniformType = GL_FLOAT;
				//shaderUniforms[aCtrlIndex].isValid = true;
			}
		}
		void createSampler2DUniform(const std::string& aName, int aCtrlIndex, int aTextureIndex = 0) {
			shaderUniforms[aCtrlIndex].name = aName;
			shaderUniforms[aCtrlIndex].textureIndex = aTextureIndex;
			shaderUniforms[aCtrlIndex].index = aCtrlIndex;
			shaderUniforms[aCtrlIndex].uniformType = GL_SAMPLER_2D;
			//shaderUniforms[aCtrlIndex].isValid = true;
		}
		void resetAutoAnimation(unsigned int aIndex) {
			shaderUniforms[aIndex].automatic = false;
			shaderUniforms[aIndex].autotime = false;
			shaderUniforms[aIndex].autobass = false;
			shaderUniforms[aIndex].automid = false;
			shaderUniforms[aIndex].autotreble = false;
			shaderUniforms[aIndex].floatValue = shaderUniforms[aIndex].defaultValue;
		}
		bool VDUniform::toggleValue(unsigned int aIndex) {
			if (shaderUniforms[aIndex].floatValue == 0.0f) {
				shaderUniforms[aIndex].floatValue = 1.0f;
			}
			else {
				shaderUniforms[aIndex].floatValue = 0.0f;
			}
			return (shaderUniforms[aIndex].floatValue == 0.0f);
		}
		bool VDUniform::toggleAuto(unsigned int aIndex) {
			
			shaderUniforms[aIndex].automatic = !shaderUniforms[aIndex].automatic;
			return shaderUniforms[aIndex].automatic;
		}
		bool VDUniform::toggleTempo(unsigned int aIndex) {
			shaderUniforms[aIndex].autotime = !shaderUniforms[aIndex].autotime;
			return shaderUniforms[aIndex].autotime;
		}
		bool VDUniform::toggleBass(unsigned int aIndex) {
			shaderUniforms[aIndex].autobass = !shaderUniforms[aIndex].autobass;
			return shaderUniforms[aIndex].autobass;
		}
		bool VDUniform::toggleMid(unsigned int aIndex) {
			shaderUniforms[aIndex].automid = !shaderUniforms[aIndex].automid;
			return shaderUniforms[aIndex].automid;
		}
		bool VDUniform::toggleTreble(unsigned int aIndex) {
			shaderUniforms[aIndex].autotreble = !shaderUniforms[aIndex].autotreble;
			return shaderUniforms[aIndex].autotreble;
		}
	private:
		// Settings
		VDSettingsRef					mVDSettings;
		std::map<int, VDUniformStruct>		shaderUniforms;
		//fs::path						mUniformsJson;
		//! read a uniforms json file 
		void							loadUniforms(const ci::DataSourceRef& source);
		int								mLastBeat = 0;

		void							floatFromJson(const ci::JsonTree& json);
		void							sampler2dFromJson(const ci::JsonTree& json);
		void							vec2FromJson(const ci::JsonTree& json);
		void							vec3FromJson(const ci::JsonTree& json);
		void							vec4FromJson(const ci::JsonTree& json);
		void							intFromJson(const ci::JsonTree& json);
		void							boolFromJson(const ci::JsonTree& json);


		void createVec2Uniform(const std::string& aName, int aCtrlIndex, vec2 aValue = vec2(0.0)) {
			shaderUniforms[aCtrlIndex].name = aName;
			shaderUniforms[aCtrlIndex].index = aCtrlIndex;
			shaderUniforms[aCtrlIndex].uniformType = GL_FLOAT_VEC2;
			//shaderUniforms[aCtrlIndex].isValid = true;
			shaderUniforms[aCtrlIndex].floatValue = aValue.x;
			//shaderUniforms[aCtrlIndex].vec2Value = aValue;
		}
		void createVec3Uniform(const std::string& aName, int aCtrlIndex, vec3 aValue = vec3(0.0)) {
			shaderUniforms[aCtrlIndex].name = aName;
			shaderUniforms[aCtrlIndex].index = aCtrlIndex;
			shaderUniforms[aCtrlIndex].uniformType = GL_FLOAT_VEC3;
			//shaderUniforms[aCtrlIndex].isValid = true;
			shaderUniforms[aCtrlIndex].floatValue = aValue.x;
			//shaderUniforms[aCtrlIndex].vec3Value = aValue;
		}
		void createVec4Uniform(const std::string& aName, int aCtrlIndex, vec4 aValue = vec4(0.0)) {
			shaderUniforms[aCtrlIndex].name = aName;
			shaderUniforms[aCtrlIndex].index = aCtrlIndex;
			shaderUniforms[aCtrlIndex].uniformType = GL_FLOAT_VEC4;
			//shaderUniforms[aCtrlIndex].isValid = true;
			shaderUniforms[aCtrlIndex].floatValue = aValue.x;
			//shaderUniforms[aCtrlIndex].vec4Value = aValue;
		}
		void createIntUniform(const std::string& aName, int aCtrlIndex, int aValue = 1) {
			shaderUniforms[aCtrlIndex].name = aName;
			shaderUniforms[aCtrlIndex].index = aCtrlIndex;
			shaderUniforms[aCtrlIndex].uniformType = GL_INT;
			//shaderUniforms[aCtrlIndex].isValid = true;
			shaderUniforms[aCtrlIndex].floatValue = aValue;
		}
		void createBoolUniform(const std::string& aName, int aCtrlIndex, bool aValue = false) {
			shaderUniforms[aCtrlIndex].name = aName;
			shaderUniforms[aCtrlIndex].minValue = 0;
			shaderUniforms[aCtrlIndex].maxValue = 1;
			shaderUniforms[aCtrlIndex].defaultValue = aValue;
			shaderUniforms[aCtrlIndex].anim = 0;
			shaderUniforms[aCtrlIndex].index = aCtrlIndex;
			shaderUniforms[aCtrlIndex].floatValue = aValue;
			shaderUniforms[aCtrlIndex].uniformType = GL_BOOL;
			//shaderUniforms[aCtrlIndex].isValid = true;
		}
		int stringToIndex(const std::string& key) {
			int rtn = -1;
			if (key == "iTime") {
				rtn = mVDSettings->ITIME;
			}
			else if (key == "r") {
				rtn = mVDSettings->IFR;
			} // 1
			// green
			else if (key == "g") {
				rtn = mVDSettings->IFG;
			} // 2
			// blue
			else if (key == "b") {
				rtn = mVDSettings->IFB;
			} // 3
			// Alpha 
			else if (key == "iAlpha") {
				rtn = mVDSettings->IFA;
			} // 4
			// red multiplier 
			else if (key == "iRedMultiplier") {
				rtn = mVDSettings->IFRX;
			} // 5
			// green multiplier 
			else if (key == "iGreenMultiplier") {
				rtn = mVDSettings->IFGX;
			} // 6
			// blue multiplier 
			else if (key == "iBlueMultiplier") {
				rtn = mVDSettings->IFBX;
			} // 7
			// gstnsmk
			else if (key == "iSobel") {
				rtn = mVDSettings->ISOBEL;
			} // 8
			// RotationSpeed
			else if (key == "iRotationSpeed") {
				rtn = mVDSettings->IROTATIONSPEED;
			} // 9
			// Steps
			else if (key == "iSteps") {
				rtn = mVDSettings->ISTEPS;
			} // 10

			// rotary
			// ratio
			else if (key == "iRatio") {
				rtn = mVDSettings->IRATIO;
			} // 11
			// zoom
			else if (key == "iZoom") {
				rtn = mVDSettings->IZOOM;
			} // 12
			// Audio multfactor 
			else if (key == "iAudioMult") {
				rtn = mVDSettings->IAUDIOX;
			} // 13
			// exposure
			else if (key == "iExposure") {
				rtn = mVDSettings->IEXPOSURE;
			} // 14
			// Pixelate
			else if (key == "iPixelate") {
				rtn = mVDSettings->IPIXELATE;
			} // 15
			// Trixels
			else if (key == "iTrixels") {
				rtn = mVDSettings->ITRIXELS;
			} // 16
			// iChromatic
			else if (key == "iChromatic") {
				rtn = mVDSettings->ICHROMATIC;
			} // 17
			// iCrossfade
			else if (key == "iCrossfade") {
				rtn = mVDSettings->IXFADE;
			} // 18
			// tempo time
			else if (key == "iTempoTime") {
				rtn = mVDSettings->ITEMPOTIME;
			} // 19
			// fps
			else if (key == "iFps") {
				rtn = mVDSettings->IFPS;
			} // 20	
			// iBpm 
			else if (key == "iBpm") {
				rtn = mVDSettings->IBPM;
			} // 21
			// Speed 
			else if (key == "speed") {
				rtn = mVDSettings->ISPEED;
			} // 22
			// slitscan / matrix (or other) Param1 
			else if (key == "iPixelX") {
				rtn = mVDSettings->IPIXELX;
			} // 23
			// slitscan / matrix(or other) Param2 
			else if (key == "iPixelY") {
				rtn = mVDSettings->IPIXELY;
			} // 24
			// delta time in seconds
			else if (key == "iDeltaTime") {
				rtn = mVDSettings->IDELTATIME;
			} // 25

			 // background red
			else if (key == "iBR") {
				rtn = mVDSettings->IBR;
			} // 26
			// background green
			else if (key == "iBG") {
				rtn = mVDSettings->IBG;
			}// 27
			// background blue
			else if (key == "iBB") {
				rtn = mVDSettings->IBB;
			} // 28

			// contour
			else if (key == "iContour") {
			rtn = mVDSettings->ICONTOUR;
			} // 30

			// weight mix fbo texture 0
			else if (key == "iWeight0") {
				rtn = mVDSettings->IWEIGHT0;
			} // 31
			// weight texture 1
			else if (key == "iWeight1") {
				rtn = mVDSettings->IWEIGHT1;
			} // 32
			// weight texture 2
			else if (key == "iWeight2") {
				rtn = mVDSettings->IWEIGHT2;
			} // 33
			// weight texture 3
			else if (key == "iWeight3") {
				rtn = mVDSettings->IWEIGHT3;
			} // 34
			// weight texture 4
			else if (key == "iWeight4") {
				rtn = mVDSettings->IWEIGHT4;
			} // 35
			// weight texture 5
			else if (key == "iWeight5") {
				rtn = mVDSettings->IWEIGHT5;
			} // 36
			// weight texture 6
			else if (key == "iWeight6") {
				rtn = mVDSettings->IWEIGHT6;
			} // 37
			// weight texture 7
			else if (key == "iWeight7") {
				rtn = mVDSettings->IWEIGHT7;
			} // 38
			// weight texture 8 
			else if (key == "iWeight8") {
				rtn = mVDSettings->IWEIGHT8;
			} // 39


			// iMouseX  
			else if (key == "iMouseX") {
				rtn = mVDSettings->IMOUSEX;
			} // 42
			// iMouseY  
			else if (key == "iMouseY") {
				rtn = mVDSettings->IMOUSEY;
			} // 43
			// iMouseZ  
			else if (key == "iMouseZ") {
				rtn = mVDSettings->IMOUSEZ;
			} // 44
			// vignette amount
			else if (key == "iVAmount") {
				rtn = mVDSettings->IVAMOUNT;
			} // 45
			// vignette falloff
			else if (key == "iVFallOff") {
				rtn = mVDSettings->IVFALLOFF;
			} // 46
			// bad tv
			else if (key == "iBadTv") {
			rtn = mVDSettings->IBADTV;
			} // 48

			// iTimeFactor
			else if (key == "iTimeFactor") {
				rtn = mVDSettings->ITIMEFACTOR;// 49
			}
			// int
			// blend mode 
			else if (key == "iBlendmode") {
				rtn = mVDSettings->IBLENDMODE;
			} // 50
			// beat 
			else if (key == "iBeat") {
				rtn = mVDSettings->IBEAT;
			} // 51
			// bar 
			else if (key == "iBar") {
				rtn = mVDSettings->IBAR;
			} // 52
			// bar 
			else if (key == "iBarBeat") {
				rtn = mVDSettings->IBARBEAT;
			} // 53		
			// fbo A
			else if (key == "iFboA") {
				rtn = mVDSettings->IFBOA;
			} // 54
			// fbo B
			else if (key == "iFboB") {
				rtn = mVDSettings->IFBOB;
			} // 55
			// iOutW
			else if (key == "iOutW") {
				rtn = mVDSettings->IOUTW;
			} // 56
			// iOutH  
			else if (key == "iOutH") {
				rtn = mVDSettings->IOUTH;
			} // 57
			// beats per bar 
			else if (key == "iBeatsPerBar") {
				rtn = mVDSettings->IBEATSPERBAR;
			} // 59

			// vec3
			// iResolutionX (should be fbowidth?) 
			else if (key == "iResolutionX") {
				rtn = mVDSettings->IRESOLUTIONX;
			} // 121
			// iResolutionY (should be fboheight?)  
			else if (key == "iResolutionY") {
				rtn = mVDSettings->IRESOLUTIONY;
			} // 122
			else if (key == "iResolution") {
				rtn = mVDSettings->IRESOLUTION;
			} // 120


			else if (key == "iColor") {
				rtn = mVDSettings->ICOLOR;
			} // 61
			else if (key == "iBackgroundColor") {
				rtn = mVDSettings->IBACKGROUNDCOLOR;
			} // 62

			// vec4
			else if (key == "iMouse") {
				rtn = mVDSettings->IMOUSE;
			}//70
			else if (key == "iDate") {
				rtn = mVDSettings->IDATE;
			}//71

			// boolean
			// invert
			// glitch
			else if (key == "iGlitch") {
				rtn = mVDSettings->IGLITCH;
			} // 81
			// vignette
			else if (key == "iVignette") {
				rtn = mVDSettings->IVIGN;
			} // 82 toggle
			// toggle
			else if (key == "iToggle") {
				rtn = mVDSettings->ITOGGLE;
			} // 83
			// invert
			else if (key == "iInvert") {
				rtn = mVDSettings->IINVERT;
			} // 86
			// greyscale 
			else if (key == "iGreyScale") {
				rtn = mVDSettings->IGREYSCALE;
			} //87

			else if (key == "iClear") {
				rtn = mVDSettings->ICLEAR;
			} // 88
			else if (key == "iDebug") {
				rtn = mVDSettings->IDEBUG;
			} // 129
			else if (key == "iXorY") {
				rtn = mVDSettings->IXORY;
			} // 130
			else if (key == "iFlipH") {
				rtn = mVDSettings->IFLIPH;
			} // 131
			else if (key == "iFlipV") {
				rtn = mVDSettings->IFLIPV;
			} // 132
			else if (key == "iFlipPostH") {
				rtn = mVDSettings->IFLIPPOSTH;
			} // 133
			else if (key == "iFlipPostV") {
				rtn = mVDSettings->IFLIPPOSTV;
			} // 134

			// 119 to 124 timefactor from midithor sos
			// floats for warps
			// srcArea 
			else if (key == "srcXLeft") {
				rtn = mVDSettings->SRCXLEFT;
			} // 160
			else if (key == "srcXRight") {
				rtn = mVDSettings->SRCXRIGHT;
			} // 161
			else if (key == "srcYLeft") {
				rtn = mVDSettings->SRCYLEFT;
			} // 162
			else if (key == "srcYRight") {
				rtn = mVDSettings->SRCYRIGHT;
			} // 163
			// iFreq0  
			else if (key == "iFreq0") {
				rtn = mVDSettings->IFREQ0;
			} // 140	
			// iFreq1  
			else if (key == "iFreq1") {
				rtn = mVDSettings->IFREQ1;
			} // 141
			// iFreq2  
			else if (key == "iFreq2") {
				rtn = mVDSettings->IFREQ2;
			} // 142
			// iFreq3  
			else if (key == "iFreq3") {
				rtn = mVDSettings->IFREQ3;
			} // 143

			// vec2
			else if (key == "resolution") {
				rtn = mVDSettings->RESOLUTION;
			} // hydra 150
			else if (key == "RENDERSIZE") {
				rtn = mVDSettings->RENDERSIZE;
			} // isf 151
			else if (key == "ciModelViewProjection") {
				rtn = 498; // TODO
			}
			else if (key == "inputImage") {
				rtn = 499; // TODO
			}
			if (rtn == -1) {
				// not found
				createFloatUniform(key, 500); // TODO
			}
			return rtn;
		}
	};
};
