#pragma once

#include "cinder/Cinder.h"
#include "cinder/app/App.h"

// json
#include "cinder/Json.h"
// Settings
#include "VDSettings.h"
//!  Uniforms
#include "VDUniform.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace videodromm
{
	// stores the pointer to the VDAnimation instance
	typedef std::shared_ptr<class VDAnimation> VDAnimationRef;

	//enum class UniformTypes { FLOAT, SAMPLER2D, VEC2, VEC3, VEC4, INT, BOOL };
	/*
	struct VDUniform
	{
		int								uniformType;
		int								index;
		float							defaultValue;
		float							floatValue;
		bool							boolValue;
		int								intValue;
		vec2							vec2Value;
		vec3							vec3Value;
		vec4							vec4Value;
		float							minValue;
		float							maxValue;
		bool							autotime;
		bool							automatic;
		bool							autobass;
		bool							automid;
		bool							autotreble;
		int								textureIndex;
		bool							isValid;
	};*/
	
	class VDAnimation {
	public:
		VDAnimation(VDSettingsRef aVDSettings);

		static VDAnimationRef			create(VDSettingsRef aVDSettings)
		{
			return shared_ptr<VDAnimation>(new VDAnimation(aVDSettings));
		}
		void							update();
		void							save();
		string							getAssetsPath() {
			return mVDSettings->mAssetsPath;
		}
		Color							getBackgroundColor() { return mBackgroundColor; };
		float							getExposure() { return mExposure; };
		void							setExposure(float aExposure);
		bool							getAutoBeatAnimation() { return mAutoBeatAnimation; };
		void							setAutoBeatAnimation(bool aAutoBeatAnimation);

		const int						mBlendModes = 28;
		void							blendRenderEnable(bool render) { mBlendRender = render; };
		// tap tempo
		bool							mUseTimeWithTempo;
		void							toggleUseTimeWithTempo() { mUseTimeWithTempo = !mUseTimeWithTempo; };
		void							useTimeWithTempo() { mUseTimeWithTempo = true; };
		bool							getUseTimeWithTempo() { return mUseTimeWithTempo; };
		float							iTempoTimeBeatPerBar;
		float							getBpm() {
			return getUniformValue(mVDSettings->IBPM);
		};
		/*void							setBpm(float aBpm) {
			//CI_LOG_W("setBpm " + toString(aBpm));

			if (aBpm > 0.0f) {
				setUniformValue(mVDSettings->IBPM, aBpm);
				setUniformValue(mVDSettings->IDELTATIME, 60 / aBpm);
			}
		};*/
		void							tapTempo();
		void							setTimeFactor(const int &aTimeFactor);
		int								getEndFrame() { return mEndFrame; };
		void							setEndFrame(int frame) { mEndFrame = frame; };
		int								mLastBeat = 0;
		// animation
		int								currentScene;
		//int							getBadTV(int frame);
		// keyboard
		bool							handleKeyDown(KeyEvent &event);
		bool							handleKeyUp(KeyEvent &event);
		// audio
		float							maxVolume;
		static const int				mWindowSize = 128; // fft window size
		float							iFreqs[mWindowSize];
		void							preventLineInCrash(); // at next launch
		void							saveLineIn();
		bool							getUseAudio() {
			return mVDSettings->mUseAudio;
		};
		bool							getUseLineIn() {
			return mVDSettings->mUseLineIn;
		};
		void							setUseLineIn(bool useLineIn) {
			mVDSettings->mUseLineIn = useLineIn;
		};
		void							toggleUseLineIn() { mVDSettings->mUseLineIn = !mVDSettings->mUseLineIn; };

		// audio
		bool							isAudioBuffered() { return mAudioBuffered; };
		void							toggleAudioBuffered() { mAudioBuffered = !mAudioBuffered; };

		// shaders
		bool							toggleValue(unsigned int aIndex) {
			return mVDUniform->toggleValue(aIndex);
		};
		bool							toggleAuto(unsigned int aIndex) {
			return mVDUniform->toggleAuto(aIndex);
		};
		bool							toggleTempo(unsigned int aIndex) {
			return mVDUniform->toggleTempo(aIndex);
		};
		bool							toggleBass(unsigned int aIndex) {
			return mVDUniform->toggleBass(aIndex);
		};
		bool							toggleMid(unsigned int aIndex) {
			return mVDUniform->toggleMid(aIndex);
		};
		bool							toggleTreble(unsigned int aIndex) {
			return mVDUniform->toggleTreble(aIndex);
		};
		void							resetAutoAnimation(unsigned int aIndex) {
			mVDUniform->resetAutoAnimation(aIndex);
		}
		
		/*bool							isExistingUniform(string aName);
		int								getUniformType(string aName);
		string							getUniformNameForIndex(int aIndex) {
			return controlIndexes[aIndex];
		};
		int								getUniformIndexForName(string aName) {
			return shaderUniforms[aName].index;
		};

		bool							setUniformValue(unsigned int aIndex, float aValue);
*/
		/*bool							setBoolUniformValueByIndex(unsigned int aIndex, bool aValue) {
			shaderUniforms[getUniformNameForIndex(aIndex)].boolValue = aValue;
			return aValue;
		}
		void							setUniformValueByName(string aName, int aValue) {
			shaderUniforms[aName].intValue = aValue;
		};
		void							setUniformValue(unsigned int aIndex, int aValue) {
			if (mVDSettings->IBEAT == aIndex) {
				if (aValue != mLastBeat) {
					mLastBeat = aValue;
					if (aValue == 0) setUniformValue(mVDSettings->IBAR, getIntUniformValueByIndex(mVDSettings->IBAR) + 1);
				}
			}
			shaderUniforms[getUniformNameForIndex(aIndex)].intValue = aValue;
		}*/
		// shaders
		int								getUniformTypeByName(const std::string& aName) {
			return mVDUniform->getUniformTypeByName(aName);
		}
		bool							isExistingUniform(const std::string& aName) { return true; }; // TODO

		/*
		string							getUniformNameForIndex(int aIndex) {
			return shaderUniforms[aIndex].name; //controlIndexes[aIndex];
		};*/
		int								getUniformIndexForName(const std::string& aName) {
			return mVDUniform->getUniformIndexForName(aName);
			//return shaderUniforms[stringToIndex(aName)].index;
		};


		void							setAnim(unsigned int aCtrl, unsigned int aAnim) {
			mVDUniform->setAnim(aCtrl, aAnim);
		}
		bool							setUniformValue(unsigned int aIndex, float aValue) {
			if (aIndex == mVDSettings->IBPM) {
				if (aValue > 0.0f) {
					mVDUniform->setUniformValue(mVDSettings->IDELTATIME, 60 / aValue);
				}
			}
			return mVDUniform->setUniformValue(aIndex, aValue);
		}

		bool							setBoolUniformValueByIndex(unsigned int aIndex, bool aValue) {
			return mVDUniform->setBoolUniformValueByIndex(aIndex, aValue);
		}
		/*void							setUniformValueByName(const std::string& aName, int aValue) {
			mVDUniform->setUniformValueByName(aName, aValue);
		};
		void							setUniformValue(unsigned int aIndex, int aValue) {
			mVDUniform->setUniformValue(aIndex, aValue);
		}*/
		void							setFloatUniformValueByName(const std::string& aName, float aValue) {
			mVDUniform->setFloatUniformValueByName(aName, aValue);
		}
		void setVec2UniformValueByName(const std::string& aName, vec2 aValue) {
			mVDUniform->setVec2UniformValueByName(aName, aValue);
		}
		void setVec2UniformValueByIndex(unsigned int aIndex, vec2 aValue) {
			mVDUniform->setVec2UniformValueByIndex(aIndex, aValue);
		}
		void setVec3UniformValueByName(const std::string& aName, vec3 aValue) {
			mVDUniform->setVec3UniformValueByName(aName, aValue);
		}
		void setVec3UniformValueByIndex(unsigned int aIndex, vec3 aValue) {
			mVDUniform->setVec3UniformValueByIndex(aIndex, aValue);
		}
		void setVec4UniformValueByName(const std::string& aName, vec4 aValue) {
			mVDUniform->setVec4UniformValueByName(aName, aValue);
		}
		void setVec4UniformValueByIndex(unsigned int aIndex, vec4 aValue) {
			mVDUniform->setVec4UniformValueByIndex(aIndex, aValue);
		}
		bool							getBoolUniformValueByIndex(unsigned int aIndex) {
			return mVDUniform->getBoolUniformValueByIndex(aIndex);
		}
		float							getMinUniformValue(unsigned int aIndex) {
			return mVDUniform->getMinUniformValue(aIndex);
		}
		float							getMaxUniformValue(unsigned int aIndex) {
			return mVDUniform->getMaxUniformValue(aIndex);
		}
		float							getMinUniformValueByName(const std::string& aName) {
			return mVDUniform->getMinUniformValueByName(aName);
		}
		float							getMaxUniformValueByName(const std::string& aName) {
			return mVDUniform->getMaxUniformValueByName(aName);
		}
		bool							getBoolUniformValueByName(const std::string& aName) {
			return mVDUniform->getBoolUniformValueByName(aName);
		}
		float							getUniformValue(unsigned int aIndex) {
			return mVDUniform->getUniformValue(aIndex);
		}
		std::string						getUniformName(unsigned int aIndex) {
			return mVDUniform->getUniformName(aIndex);
		}
		int								getUniformAnim(unsigned int aIndex) {
			return mVDUniform->getUniformAnim(aIndex);
		}
		float							getFloatUniformDefaultValueByIndex(unsigned int aIndex) {
			return mVDUniform->getFloatUniformDefaultValueByIndex(aIndex);
		}
		int								getIntUniformValueByIndex(unsigned int aIndex) {
			return mVDUniform->getIntUniformValueByIndex(aIndex);
		}
		int								getSampler2DUniformValueByName(const std::string& aName) {
			return mVDUniform->getSampler2DUniformValueByName(aName);
		}
		float							getUniformValueByName(const std::string& aName) {
			return mVDUniform->getUniformValueByName(aName);
		}

		vec2							getVec2UniformValueByName(const std::string& aName) {
			return mVDUniform->getVec2UniformValueByName(aName);
		}
		vec3							getVec3UniformValueByName(const std::string& aName) {
			return mVDUniform->getVec3UniformValueByName(aName);
		}
		vec4							getVec4UniformValueByName(const std::string& aName) {
			return mVDUniform->getVec4UniformValueByName(aName);
		}
		int								getIntUniformValueByName(const std::string& aName) {
			return mVDUniform->getIntUniformValueByName(aName);
		};

		// mix fbo
		bool							isFlipH() { return getBoolUniformValueByIndex(mVDSettings->IFLIPH); };
		bool							isFlipV() { return getBoolUniformValueByIndex(mVDSettings->IFLIPV); };
		void							flipH() { setBoolUniformValueByIndex(mVDSettings->IFLIPH, !getBoolUniformValueByIndex(mVDSettings->IFLIPH)); };
		void							flipV() { setBoolUniformValueByIndex(mVDSettings->IFLIPV, !getBoolUniformValueByIndex(mVDSettings->IFLIPV)); };

		unsigned int					getBlendModesCount() { return mBlendModes; };
		bool							renderBlend() { return mBlendRender; };

		// timed animation
		int								mEndFrame;
		int								getFreqIndexSize() { return freqIndexes.size(); };
		int								getFreqIndex(unsigned int aFreqIndex) { return freqIndexes[aFreqIndex]; };
		void							setFreqIndex(unsigned int aFreqIndex, unsigned int aFreq) { freqIndexes[aFreqIndex] = aFreq; };
		//float							getFreq(unsigned int aFreqIndex) { return iFreqs[freqIndexes[aFreqIndex]]; };
		// public for hydra
		void							createFloatUniform(string aName, int aCtrlIndex, float aValue = 0.01f, float aMin = 0.0f, float aMax = 1.0f) {
			mVDUniform->createFloatUniform(aName, aCtrlIndex, aValue, aMin, aMax);
		};
		void							createSampler2DUniform(string aName, int aCtrlIndex, int aTextureIndex = 0) {
			mVDUniform->createSampler2DUniform(aName, aCtrlIndex, aTextureIndex);
		};
	private:
		// Settings
		VDSettingsRef					mVDSettings;
		//! Uniforms
		VDUniformRef					mVDUniform;
		map<int, int>					freqIndexes;
		bool							mAudioBuffered;
		// Live json params
		//fs::path						mJsonFilePath;
		//Parameter <
		Color							mBackgroundColor;
		float							mExposure;
		string							mText;
		bool							mAutoBeatAnimation;
		////Parameter >
		// shaders
		//map<int, string>				controlIndexes;
		//map<string, VDUniform>			shaderUniforms;
		//! read a uniforms json file 
		/*void							loadUniforms(const ci::DataSourceRef &source);
		void							floatFromJson(const ci::JsonTree &json);
		void							sampler2dFromJson(const ci::JsonTree &json);
		void							vec2FromJson(const ci::JsonTree &json);
		void							vec3FromJson(const ci::JsonTree &json);
		void							vec4FromJson(const ci::JsonTree &json);
		void							intFromJson(const ci::JsonTree &json);
		void							boolFromJson(const ci::JsonTree &json);*/
		fs::path						mUniformsJson;


/*
		void							createVec2Uniform(string aName, int aCtrlIndex, vec2 aValue = vec2(0.0));
		void							createVec3Uniform(string aName, int aCtrlIndex, vec3 aValue = vec3(0.0));
		void							createVec4Uniform(string aName, int aCtrlIndex, vec4 aValue = vec4(0.0));
		void							createIntUniform(string aName, int aCtrlIndex, int aValue = 1);
		void							createBoolUniform(string aName, int aCtrlIndex, bool aValue = false);
		
		//! write a uniforms json file
		void							saveUniforms();
		ci::JsonTree					uniformToJson(int i);*/

		// time
		ci::Timer						mTimer;
		std::deque <double>				buffer;
		void							calculateTempo();
		int								counter;
		double							averageTime;
		double							currentTime;
		double							startTime;
		float							previousTime;
		float							previousTimeBeatPerBar;
		JsonTree						mData;
		void							loadAnimation();
		void							saveAnimation();
		int								mLastBar = 0;
		std::unordered_map<int, float>	mBadTV;
		//bool							mFlipH;
		//bool							mFlipV;
		bool							mBlendRender;
		// timed animation
		//float							mBpm;

	};
}
