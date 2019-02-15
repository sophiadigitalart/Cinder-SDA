#include "SDAAnimation.h"

using namespace SophiaDigitalArt;

SDAAnimation::SDAAnimation(SDASettingsRef aSDASettings) {
	mSDASettings = aSDASettings;

	mBlendRender = false;
	//audio
	mAudioBuffered = false;
	//setUseLineIn(true);
	maxVolume = 0.0f;
	for (int i = 0; i < 7; i++)
	{
		freqIndexes[i] = i * 7;
	}
	for (int i = 0; i < mWindowSize; i++)
	{
		iFreqs[i] = i;
	}
	// live json params
	mJsonFilePath = app::getAssetPath("") / mSDASettings->mAssetsPath / "live_params.json";
	JsonBag::add(&mBackgroundColor, "background_color");
	JsonBag::add(&mExposure, "exposure", []() {
		app::console() << "Updated exposure" << endl;

	});
	JsonBag::add(&mText, "text", []() {
		app::console() << "Updated text" << endl;
	});
	mAutoBeatAnimation = true;
	JsonBag::add(&mAutoBeatAnimation, "autobeatanimation");
	currentScene = 0;

	previousTime = 0.0f;
	counter = 0;
	iTimeFactor = 1.0f;
	// tempo
	mUseTimeWithTempo = false;
	// init timer
	mTimer.start();
	startTime = currentTime = mTimer.getSeconds();
	//mBpm = 166;
	//iDeltaTime = 60 / mBpm;//mTempo;
	//setFloatUniformValueByIndex(mSDASettings->IBPM, 166.0f);
	iDeltaTime = 60.0f / 166.0f;
	//iBar = 0;
	//iBadTvRunning = false;
	//int ctrl;

	mUniformsJson = getAssetPath("") / mSDASettings->mAssetsPath / "uniforms.json";
	if (fs::exists(mUniformsJson)) {
		loadUniforms(loadFile(mUniformsJson));
	}
	else {
		// global time in seconds
		createFloatUniform("iTime", mSDASettings->ITIME, 0.0f); // 0
		// sliders
		// red
		createFloatUniform("iFR", mSDASettings->IFR, 1.0f); // 1
		// green
		createFloatUniform("iFG", mSDASettings->IFG, 0.3f); // 2
		// blue
		createFloatUniform("iFB", mSDASettings->IFB, 0.0f); // 3
		// Alpha 
		createFloatUniform("iAlpha", mSDASettings->IFA, 1.0f); // 4
		// red multiplier 
		createFloatUniform("iRedMultiplier", mSDASettings->IFRX, 1.0f, 0.0f, 3.0f); // 5
		// green multiplier 
		createFloatUniform("iGreenMultiplier", mSDASettings->IFGX, 1.0f, 0.0f, 3.0f); // 6
		// blue multiplier 
		createFloatUniform("iBlueMultiplier", mSDASettings->IFBX, 1.0f, 0.0f, 3.0f); // 7
		// gstnsmk
		createFloatUniform("iSobel", mSDASettings->ISOBEL, 0.02f, 0.02f, 1.0f); // 8
		// bad tv
		createFloatUniform("iBadTv", mSDASettings->IBADTV, 0.0f, 0.0f, 5.0f); // 9
		// Steps
		createFloatUniform("iSteps", mSDASettings->ISTEPS, 16.0f, 1.0f, 128.0f); // 10

		// rotary
		// ratio
		createFloatUniform("iRatio", mSDASettings->IRATIO, 20.0f, 0.00000000001f, 20.0f); // 11
		// zoom
		createFloatUniform("iZoom", mSDASettings->IZOOM, 1.0f, -3.0f, 3.0f); // 12
		// Audio multfactor 
		createFloatUniform("iAudioMult", mSDASettings->IAUDIOX, 1.0f, 0.01f, 12.0f); // 13
		// exposure
		createFloatUniform("iExposure", mSDASettings->IEXPOSURE, 1.0f, 0.0f, 3.0f); // 14
		// Pixelate
		createFloatUniform("iPixelate", mSDASettings->IPIXELATE, 1.0f, 0.01f); // 15
		// Trixels
		createFloatUniform("iTrixels", mSDASettings->ITRIXELS, 0.0f); // 16
		// iChromatic
		createFloatUniform("iChromatic", mSDASettings->ICHROMATIC, 0.0f, 0.000000001f); // 17
		// iCrossfade
		createFloatUniform("iCrossfade", mSDASettings->IXFADE, 1.0f); // 18
		// tempo time
		createFloatUniform("iTempoTime", mSDASettings->ITEMPOTIME, 0.1f); // 19
		// fps
		createFloatUniform("iFps", mSDASettings->IFPS, 60.0f, 0.0f, 500.0f); // 20	
		// iBpm 
		createFloatUniform("iBpm", mSDASettings->IBPM, 165.0f, 0.000000001f, 400.0f); // 21
		// Speed 
		createFloatUniform("iSpeed", mSDASettings->ISPEED, 12.0f, 0.01f, 12.0f); // 22
		// slitscan (or other) Param1 
		createFloatUniform("iParam1", mSDASettings->IPARAM1, 1.0f, 0.01f, 100.0f); // 23
		// slitscan (or other) Param2 
		createFloatUniform("iParam2", mSDASettings->IPARAM2, 1.0f, 0.01f, 100.0f); // 24
		// iFreq0  
		createFloatUniform("iFreq0", mSDASettings->IFREQ0, 0.0f, 0.01f, 256.0f); // 25 
		// iFreq1  
		createFloatUniform("iFreq1", mSDASettings->IFREQ1, 0.0f, 0.01f, 256.0f); // 26
		// iFreq2  
		createFloatUniform("iFreq2", mSDASettings->IFREQ2, 0.0f, 0.01f, 256.0f); // 27
		// iFreq3  
		createFloatUniform("iFreq3", mSDASettings->IFREQ3, 0.0f, 0.01f, 256.0f); // 28
		// iResolutionX (should be fbowidth) 
		createFloatUniform("iResolutionX", mSDASettings->IRESX, mSDASettings->mFboWidth, 0.01f, 2280.0f); // 29
		// iResolutionY (should be fboheight)  
		createFloatUniform("iResolutionY", mSDASettings->IRESY, mSDASettings->mFboHeight, 0.01f, 2800.0f); // 30

		// TODO: double 
		// weight mix fbo texture 0
		createFloatUniform("iWeight0", mSDASettings->IWEIGHT0, 1.0f); // 31
		// weight texture 1
		createFloatUniform("iWeight1", mSDASettings->IWEIGHT1, 0.0f); // 32
		// weight texture 2
		createFloatUniform("iWeight2", mSDASettings->IWEIGHT2, 0.0f); // 33
		// weight texture 3
		createFloatUniform("iWeight3", mSDASettings->IWEIGHT3, 0.0f); // 34
		// weight texture 4
		createFloatUniform("iWeight4", mSDASettings->IWEIGHT4, 0.0f); // 35
		// background red
		createFloatUniform("iBR", mSDASettings->IBR, 0.1f); // 36
		// background green
		createFloatUniform("iBG", mSDASettings->IBG, 0.5f); // 37
		// background blue
		createFloatUniform("iBB", mSDASettings->IBB, 0.1f); // 38
		// background alpha
		createFloatUniform("iBA", mSDASettings->IBA, 0.2f); // 39
		// contour
		createFloatUniform("iContour", mSDASettings->ICONTOUR, 0.0f, 0.0f, 0.5f); // 40
		// RotationSpeed
		createFloatUniform("iRotationSpeed", mSDASettings->IROTATIONSPEED, 0.0f, -2.0f, 2.0f); // 41
	
		// iMouseX  
		createFloatUniform("iMouseX", mSDASettings->IMOUSEX, 320.0f, 0.0f, 1280.0f); // 42
		// iMouseY  
		createFloatUniform("iMouseY", mSDASettings->IMOUSEY, 240.0f, 0.0f, 800.0f); // 43
		// iMouseZ  
		createFloatUniform("iMouseZ", mSDASettings->IMOUSEZ, 0.0f, 0.0f, 1.0f); // 44
		// vignette amount
		createFloatUniform("iVAmount", mSDASettings->IVAMOUNT, 0.91f, 0.0f, 1.0f); // 45
		// vignette falloff
		createFloatUniform("iVFallOff", mSDASettings->IVFALLOFF, 0.31f, 0.0f, 1.0f); // 46

		// int
		// blend mode 
		createIntUniform("iBlendmode", 50, 0);
		// greyscale 
		createIntUniform("iGreyScale", 51, 0);
		// current beat
		createIntUniform("iPhase", mSDASettings->IPHASE, 0); // 52
		// beats per bar 
		createIntUniform("iBeatsPerBar", 53, 4);

		// vec3
		createVec3Uniform("iResolution", 60, vec3(getFloatUniformValueByName("iResolutionX"), getFloatUniformValueByName("iResolutionY"), 1.0));
		createVec3Uniform("iColor", 61, vec3(1.0, 0.5, 0.0));
		createVec3Uniform("iBackgroundColor", 62);
		//createVec3Uniform("iChannelResolution[0]", 63, vec3(mSDASettings->mFboWidth, mSDASettings->mFboHeight, 1.0));

		// vec4
		createVec4Uniform("iMouse", 70, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("iDate", 71, vec4(2018.0f, 12.0f, 1.0f, 5.0f));

		// boolean
		// invert
		// glitch
		createBoolUniform("iGlitch", mSDASettings->IGLITCH); // 81
		// vignette
		createBoolUniform("iVignette", mSDASettings->IVIGN); // 82 toggle
		// toggle
		createBoolUniform("iToggle", mSDASettings->ITOGGLE); // 83
		// invert
		createBoolUniform("iInvert", mSDASettings->IINVERT); // 86
		createBoolUniform("iXorY", mSDASettings->IXORY); // was 87
		createBoolUniform("iFlipH", mSDASettings->IFLIPH); // 100 toggle was 90
		createBoolUniform("iFlipV", mSDASettings->IFLIPV); // 103 toggle was 92

		// vec4 kinect2
		createVec4Uniform("iSpineBase", 200, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("SpineMid", 201, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("Neck", 202, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("Head", 203, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("ShldrL", 204, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("ElbowL", 205, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("WristL", 206, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("HandL", 207, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("ShldrR", 208, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("ElbowR", 209, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("WristR", 210, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("HandR", 211, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("HipL", 212, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("KneeL", 213, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("AnkleL", 214, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("FootL", 215, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("HipR", 216, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("KneeR", 217, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("AnkleR", 218, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("FootR", 219, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("SpineShldr", 220, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("HandTipL", 221, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("ThumbL", 222, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("HandTipR", 223, vec4(320.0f, 240.0f, 0.0f, 0.0f));
		createVec4Uniform("ThumbR", 224, vec4(320.0f, 240.0f, 0.0f, 0.0f));


	}
	// textures
	for (size_t i = 0; i < 8; i++)
	{
		createSampler2DUniform("iChannel" + toString(i), 300 + i, i);// TODO verify doesn't mess up type (uint!)
	}
	// iRHandX  
	//createFloatUniform("iRHandX", mSDASettings->IRHANDX, 320.0f, 0.0f, 1280.0f);
	//// iRHandY  
	//createFloatUniform("iRHandY", mSDASettings->IRHANDY, 240.0f, 0.0f, 800.0f);
	//// iLHandX  
	//createFloatUniform("iLHandX", mSDASettings->ILHANDX, 320.0f, 0.0f, 1280.0f);
	//// iLHandY  
	//createFloatUniform("iLHandY", mSDASettings->ILHANDY, 240.0f, 0.0f, 800.0f);

	load();
	loadAnimation();
	CI_LOG_V("SDAAnimation, iResX:" + toString(getFloatUniformValueByIndex(29)));
	CI_LOG_V("SDAAnimation, iResY:" + toString(getFloatUniformValueByIndex(30)));

	setVec3UniformValueByIndex(60, vec3(getFloatUniformValueByIndex(29), getFloatUniformValueByIndex(30), 1.0));
	CI_LOG_V("SDAAnimation, iResolution:" + toString(shaderUniforms[getUniformNameForIndex(60)].vec3Value));

}
void SDAAnimation::loadUniforms(const ci::DataSourceRef &source) {

	JsonTree json(source);

	// try to load the specified json file
	if (json.hasChild("uniforms")) {
		JsonTree u(json.getChild("uniforms"));

		// iterate uniforms
		for (size_t i = 0; i < u.getNumChildren(); i++) {
			JsonTree child(u.getChild(i));

			if (child.hasChild("uniform")) {
				JsonTree w(child.getChild("uniform"));
				// create uniform of the correct type
				int uniformType = (w.hasChild("type")) ? w.getValueForKey<int>("type") : 0;
				switch (uniformType) {
				case 0:
					//float
					floatFromJson(child);
					break;
				case 1:
					// sampler2d
					sampler2dFromJson(child);
					break;
				case 2:
					// vec2
					vec2FromJson(child);
					break;
				case 3:
					// vec3
					vec3FromJson(child);
					break;
				case 4:
					// vec4
					vec4FromJson(child);
					break;
				case 5:
					// int
					intFromJson(child);
					break;
				case 6:
					// boolean
					boolFromJson(child);
					break;
				}
			}
		}
	}
}
void SDAAnimation::floatFromJson(const ci::JsonTree &json) {
	string jName;
	int jCtrlIndex;
	float jValue, jMin, jMax;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 249;
		jValue = (u.hasChild("value")) ? u.getValueForKey<float>("value") : 0.01f;
		jMin = (u.hasChild("min")) ? u.getValueForKey<float>("min") : 0.0f;
		jMax = (u.hasChild("max")) ? u.getValueForKey<float>("max") : 1.0f;
		createFloatUniform(jName, jCtrlIndex, jValue, jMin, jMax);
	}
}
void SDAAnimation::sampler2dFromJson(const ci::JsonTree &json) {
	string jName;
	int jCtrlIndex;
	int jTextureIndex;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 250;
		jTextureIndex = (u.hasChild("textureindex")) ? u.getValueForKey<int>("textureindex") : 0;;
		createSampler2DUniform(jName, jTextureIndex);
	}
}
void SDAAnimation::vec2FromJson(const ci::JsonTree &json) {
	string jName;
	int jCtrlIndex;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 251;
		createVec2Uniform(jName, jCtrlIndex);
	}
}
void SDAAnimation::vec3FromJson(const ci::JsonTree &json) {
	string jName;
	int jCtrlIndex;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 252;
		createVec3Uniform(jName, jCtrlIndex);
	}
}
void SDAAnimation::vec4FromJson(const ci::JsonTree &json) {
	string jName;
	int jCtrlIndex;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 253;
		createVec4Uniform(jName, jCtrlIndex);
	}
}
void SDAAnimation::intFromJson(const ci::JsonTree &json) {
	string jName;
	int jCtrlIndex, jValue;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 254;
		jValue = (u.hasChild("value")) ? u.getValueForKey<int>("value") : 1;
		createIntUniform(jName, jCtrlIndex, jValue);
	}

}
void SDAAnimation::boolFromJson(const ci::JsonTree &json) {
	string jName;
	int jCtrlIndex;
	bool jValue;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 255;
		jValue = (u.hasChild("value")) ? u.getValueForKey<bool>("value") : false;
		createBoolUniform(jName, jCtrlIndex, jValue);
	}
}
//! uniform to json
JsonTree SDAAnimation::uniformToJson(int i)
{
	stringstream svec4;
	JsonTree		json;
	string s = controlIndexes[i];

	JsonTree u = JsonTree::makeArray("uniform");
	// common
	int uniformType = shaderUniforms[s].uniformType;
	u.addChild(ci::JsonTree("type", uniformType));
	u.addChild(ci::JsonTree("name", s));
	u.addChild(ci::JsonTree("index", i));
	// type specific 
	switch (uniformType) {
	case 0:
		//float
		u.addChild(ci::JsonTree("value", shaderUniforms[s].defaultValue));
		u.addChild(ci::JsonTree("min", shaderUniforms[s].minValue));
		u.addChild(ci::JsonTree("max", shaderUniforms[s].maxValue));
		break;
	case 1:
		// sampler2d
		u.addChild(ci::JsonTree("textureindex", shaderUniforms[s].textureIndex));
		break;
	case 4:
		// vec4
		svec4 << toString(shaderUniforms[s].vec4Value.x) << "," << toString(shaderUniforms[s].vec4Value.y);
		svec4 << "," << toString(shaderUniforms[s].vec4Value.z) << "," << toString(shaderUniforms[s].vec4Value.w);
		u.addChild(ci::JsonTree("value", svec4.str()));
		break;
	case 5:
		// int
		u.addChild(ci::JsonTree("value", shaderUniforms[s].intValue));
		break;
	case 6:
		// boolean
		u.addChild(ci::JsonTree("value", shaderUniforms[s].boolValue));
		break;
	default:
		break;
	}

	json.pushBack(u);
	return json;
}
void SDAAnimation::saveUniforms()
{
	string jName;
	int jCtrlIndex;
	float jMin, jMax;
	JsonTree		json;
	// create uniforms json
	JsonTree uniformsJson = JsonTree::makeArray("uniforms");

	for (unsigned i = 0; i < controlIndexes.size(); ++i) {
		JsonTree		u(uniformToJson(i));
		// create <uniform>
		uniformsJson.pushBack(u);
	}
	// write file
	json.pushBack(uniformsJson);
	json.write(mUniformsJson);
}

void SDAAnimation::createFloatUniform(string aName, int aCtrlIndex, float aValue, float aMin, float aMax) {
	controlIndexes[aCtrlIndex] = aName;
	shaderUniforms[aName].minValue = aMin;
	shaderUniforms[aName].maxValue = aMax;
	shaderUniforms[aName].defaultValue = aValue;
	shaderUniforms[aName].boolValue = false;
	shaderUniforms[aName].autotime = false;
	shaderUniforms[aName].automatic = false;
	shaderUniforms[aName].index = aCtrlIndex;
	shaderUniforms[aName].floatValue = aValue;
	shaderUniforms[aName].uniformType = 0;
	shaderUniforms[aName].isValid = true;
}
void SDAAnimation::createSampler2DUniform(string aName, int aCtrlIndex, int aTextureIndex) {
	shaderUniforms[aName].textureIndex = aTextureIndex;
	shaderUniforms[aName].index = aCtrlIndex;
	shaderUniforms[aName].uniformType = 1;
	shaderUniforms[aName].isValid = true;
}
void SDAAnimation::createVec2Uniform(string aName, int aCtrlIndex, vec2 aValue) {
	controlIndexes[aCtrlIndex] = aName;
	shaderUniforms[aName].index = aCtrlIndex;
	shaderUniforms[aName].uniformType = 2;
	shaderUniforms[aName].isValid = true;
	shaderUniforms[aName].vec2Value = aValue;
}
void SDAAnimation::createVec3Uniform(string aName, int aCtrlIndex, vec3 aValue) {
	controlIndexes[aCtrlIndex] = aName;
	shaderUniforms[aName].index = aCtrlIndex;
	shaderUniforms[aName].uniformType = 3;
	shaderUniforms[aName].isValid = true;
	shaderUniforms[aName].vec3Value = aValue;
}
void SDAAnimation::createVec4Uniform(string aName, int aCtrlIndex, vec4 aValue) {
	controlIndexes[aCtrlIndex] = aName;
	shaderUniforms[aName].index = aCtrlIndex;
	shaderUniforms[aName].uniformType = 4;
	shaderUniforms[aName].isValid = true;
	shaderUniforms[aName].vec4Value = aValue;
}
void SDAAnimation::createIntUniform(string aName, int aCtrlIndex, int aValue) {
	controlIndexes[aCtrlIndex] = aName;
	shaderUniforms[aName].index = aCtrlIndex;
	shaderUniforms[aName].uniformType = 5;
	shaderUniforms[aName].isValid = true;
	shaderUniforms[aName].intValue = aValue;
}
void SDAAnimation::createBoolUniform(string aName, int aCtrlIndex, bool aValue) {
	controlIndexes[aCtrlIndex] = aName;
	shaderUniforms[aName].minValue = 0;
	shaderUniforms[aName].maxValue = 1;
	shaderUniforms[aName].defaultValue = aValue;
	shaderUniforms[aName].boolValue = aValue;
	shaderUniforms[aName].autotime = false;
	shaderUniforms[aName].automatic = false;
	shaderUniforms[aName].index = aCtrlIndex;
	shaderUniforms[aName].floatValue = aValue;
	shaderUniforms[aName].uniformType = 6;
	shaderUniforms[aName].isValid = true;
}
string SDAAnimation::getUniformNameForIndex(int aIndex) {
	return controlIndexes[aIndex];
}
/*bool SDAAnimation::hasFloatChanged(int aIndex) {
	if (shaderUniforms[getUniformNameForIndex(aIndex)].floatValue != controlValues[aIndex]) {
	//CI_LOG_V("hasFloatChanged, aIndex:" + toString(aIndex));
	CI_LOG_V("hasFloatChanged, shaderUniforms[getUniformNameForIndex(aIndex)].floatValue:" + toString(shaderUniforms[getUniformNameForIndex(aIndex)].floatValue));
	CI_LOG_V("hasFloatChanged, controlValues[aIndex]:" + toString(controlValues[aIndex]));
	//CI_LOG_W("hasFloatChanged, getUniformNameForIndex(aIndex):" + toString(getUniformNameForIndex(aIndex)));
	}
	return (shaderUniforms[getUniformNameForIndex(aIndex)].floatValue != controlValues[aIndex]);
	}*/
bool SDAAnimation::toggleValue(unsigned int aIndex) {
	shaderUniforms[getUniformNameForIndex(aIndex)].boolValue = !shaderUniforms[getUniformNameForIndex(aIndex)].boolValue;
	return shaderUniforms[getUniformNameForIndex(aIndex)].boolValue;
}
bool SDAAnimation::toggleAuto(unsigned int aIndex) {
	shaderUniforms[getUniformNameForIndex(aIndex)].automatic = !shaderUniforms[getUniformNameForIndex(aIndex)].automatic;
	return shaderUniforms[getUniformNameForIndex(aIndex)].automatic;
}
bool SDAAnimation::toggleTempo(unsigned int aIndex) {
	shaderUniforms[getUniformNameForIndex(aIndex)].autotime = !shaderUniforms[getUniformNameForIndex(aIndex)].autotime;
	return shaderUniforms[getUniformNameForIndex(aIndex)].autotime;
}
void SDAAnimation::resetAutoAnimation(unsigned int aIndex) {
	shaderUniforms[getUniformNameForIndex(aIndex)].automatic = false;
	shaderUniforms[getUniformNameForIndex(aIndex)].autotime = false;
	shaderUniforms[getUniformNameForIndex(aIndex)].floatValue = shaderUniforms[getUniformNameForIndex(aIndex)].defaultValue;
}

bool SDAAnimation::setFloatUniformValueByIndex(unsigned int aIndex, float aValue) {
	bool rtn = false;
	// we can't change iTime at index 0
	if (aIndex > 0) {
		/*if (aIndex == 31) {
			CI_LOG_V("old value " + toString(shaderUniforms[getUniformNameForIndex(aIndex)].floatValue) + " newvalue " + toString(aValue));
		}*/
		if (shaderUniforms[getUniformNameForIndex(aIndex)].floatValue != aValue) {
			if (aValue >= shaderUniforms[getUniformNameForIndex(aIndex)].minValue && aValue <= shaderUniforms[getUniformNameForIndex(aIndex)].maxValue) {
				shaderUniforms[getUniformNameForIndex(aIndex)].floatValue = aValue;
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
	return rtn;
}

bool SDAAnimation::isExistingUniform(string aName) {
	return shaderUniforms[aName].isValid;
}
int SDAAnimation::getUniformType(string aName) {
	return shaderUniforms[aName].uniformType;
}
void SDAAnimation::load() {
	// Create json file if it doesn't already exist.
#if defined( CINDER_MSW )
	if (fs::exists(mJsonFilePath)) {
		bag()->load(mJsonFilePath);
	}
	else {
		bag()->save(mJsonFilePath);
		bag()->load(mJsonFilePath);
	}
#endif
}
void SDAAnimation::save() {
#if defined( CINDER_MSW )
	bag()->save(mJsonFilePath);
	saveAnimation();
	saveUniforms();
#endif
}
void SDAAnimation::saveAnimation() {
	// save 
	fs::path mJsonFilePath = app::getAssetPath("") / mSDASettings->mAssetsPath / "animation.json";
	JsonTree doc;
	JsonTree badtv = JsonTree::makeArray("badtv");

	for (const auto& item : mBadTV) {
		if (item.second > 0.0001) badtv.addChild(ci::JsonTree(ci::toString(item.first), ci::toString(item.second)));
	}

	doc.pushBack(badtv);
	doc.write(writeFile(mJsonFilePath), JsonTree::WriteOptions());
	// backup save
	/*string fileName = "animation" + toString(getElapsedFrames()) + ".json";
	mJsonFilePath = app::getAssetPath("") / mSDASettings->mAssetsPath / fileName;
	doc.write(writeFile(mJsonFilePath), JsonTree::WriteOptions());*/
}
void SDAAnimation::loadAnimation() {

	fs::path mJsonFilePath = app::getAssetPath("") / mSDASettings->mAssetsPath / "animation.json";
	// Create json file if it doesn't already exist.
	if (!fs::exists(mJsonFilePath)) {
		std::ofstream oStream(mJsonFilePath.string());
		oStream.close();
	}
	if (!fs::exists(mJsonFilePath)) {
		return;
	}
	try {
		JsonTree doc(loadFile(mJsonFilePath));
		JsonTree badtv(doc.getChild("badtv"));
		for (JsonTree::ConstIter item = badtv.begin(); item != badtv.end(); ++item) {
			const auto& key = std::stoi(item->getKey());
			const auto& value = item->getValue<float>();
			mBadTV[key] = value;

		}
	}
	catch (const JsonTree::ExcJsonParserError&) {
		CI_LOG_W("Failed to parse json file.");
	}
}

void SDAAnimation::setExposure(float aExposure) {
	mExposure = aExposure;
}
void SDAAnimation::setAutoBeatAnimation(bool aAutoBeatAnimation) {
	mAutoBeatAnimation = aAutoBeatAnimation;
}
bool SDAAnimation::handleKeyDown(KeyEvent &event)
{
	//float newValue;
	bool handled = true;
	switch (event.getCode()) {
	case KeyEvent::KEY_s:
		// save animation
		save();
		break;
	case KeyEvent::KEY_a:
		// save badtv keyframe
		mBadTV[getElapsedFrames() - 10] = 1.0f;
		//iBadTvRunning = true;
		// duration = 0.2
		shaderUniforms["iBadTv"].floatValue = 5.0f;
		//timeline().apply(&mSDASettings->iBadTv, 60.0f, 0.0f, 0.2f, EaseInCubic());
		break;
	case KeyEvent::KEY_d:
		// save end keyframe
		setEndFrame(getElapsedFrames() - 10);
		break;

		//case KeyEvent::KEY_x:
	case KeyEvent::KEY_y:
		mSDASettings->iXorY = !mSDASettings->iXorY;
		break;

	default:
		handled = false;
	}
	event.setHandled(handled);

	return event.isHandled();
}
bool SDAAnimation::handleKeyUp(KeyEvent &event)
{
	bool handled = true;
	switch (event.getCode()) {
	case KeyEvent::KEY_a:
		// save badtv keyframe
		mBadTV[getElapsedFrames()] = 0.001f;
		shaderUniforms["iBadTv"].floatValue = 0.0f;
		break;

	default:
		handled = false;
	}
	event.setHandled(handled);

	return event.isHandled();
}

void SDAAnimation::update() {

	if (mBadTV[getElapsedFrames()] == 0) {
		// TODO check shaderUniforms["iBadTv"].floatValue = 0.0f;
	}
	else {
		// duration = 0.2
		//timeline().apply(&mSDASettings->iBadTv, 60.0f, 0.0f, 0.2f, EaseInCubic());
		shaderUniforms["iBadTv"].floatValue = 5.0f;
	}

	mSDASettings->iChannelTime[0] = getElapsedSeconds();
	mSDASettings->iChannelTime[1] = getElapsedSeconds() - 1;
	mSDASettings->iChannelTime[2] = getElapsedSeconds() - 2;
	mSDASettings->iChannelTime[3] = getElapsedSeconds() - 3;
	// iTime
	if (mUseTimeWithTempo)
	{
		shaderUniforms["iTime"].floatValue = shaderUniforms["iTempoTime"].floatValue*iTimeFactor;
	}
	else
	{
		shaderUniforms["iTime"].floatValue = getElapsedSeconds();
	}
	shaderUniforms["iTime"].floatValue *= mSDASettings->iSpeedMultiplier;
	// iDate
	time_t now = time(0);
	tm *   t = gmtime(&now);
	shaderUniforms["iDate"].vec4Value = vec4(float(t->tm_year + 1900), float(t->tm_mon + 1), float(t->tm_mday), float(t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec));

#pragma region animation

	currentTime = mTimer.getSeconds();
	// TODO check bounds
	if (mAutoBeatAnimation) mSDASettings->liveMeter = maxVolume * 2;

	int time = (currentTime - startTime)*1000000.0;
	int elapsed = iDeltaTime*1000000.0;
	int elapsedBeatPerBar = iDeltaTime / (shaderUniforms["iBeatsPerBar"].intValue + 1)*1000000.0;
	if (elapsedBeatPerBar > 0)
	{
		double moduloBeatPerBar = (time % elapsedBeatPerBar) / 1000000.0;
		iTempoTimeBeatPerBar = (float)moduloBeatPerBar;
		if (iTempoTimeBeatPerBar < previousTimeBeatPerBar)
		{
			if (shaderUniforms["iPhase"].intValue > shaderUniforms["iBeatsPerBar"].intValue ) shaderUniforms["iPhase"].intValue = 0;
			shaderUniforms["iPhase"].intValue++;
		}
		previousTimeBeatPerBar = iTempoTimeBeatPerBar;
	}
	if (elapsed > 0)
	{
		double modulo = (time % elapsed) / 1000000.0;
		shaderUniforms["iTempoTime"].floatValue = (float)abs(modulo);
		if (shaderUniforms["iTempoTime"].floatValue < previousTime)
		{
			//iBar++;
			//if (mAutoBeatAnimation) mSDASettings->iPhase++;
		}
		previousTime = shaderUniforms["iTempoTime"].floatValue;

		// TODO (modulo < 0.1) ? tempoMvg->setNameColor(ColorA::white()) : tempoMvg->setNameColor(UIController::DEFAULT_NAME_COLOR);
		for (unsigned int anim = 1; anim < 29; anim++)
		{
			if (shaderUniforms[getUniformNameForIndex(anim)].autotime)
			{
				setFloatUniformValueByIndex(anim, (modulo < 0.1) ? shaderUniforms[getUniformNameForIndex(anim)].maxValue : shaderUniforms[getUniformNameForIndex(anim)].minValue);
			}
			else
			{
				if (shaderUniforms[getUniformNameForIndex(anim)].automatic) {
					setFloatUniformValueByIndex(anim, lmap<float>(shaderUniforms["iTempoTime"].floatValue, 0.00001, iDeltaTime, shaderUniforms[getUniformNameForIndex(anim)].minValue, shaderUniforms[getUniformNameForIndex(anim)].maxValue));
				}
			}
		}

		// foreground color vec3 update
		shaderUniforms["iColor"].vec3Value = vec3(shaderUniforms[getUniformNameForIndex(mSDASettings->IFR)].floatValue, shaderUniforms[getUniformNameForIndex(mSDASettings->IFG)].floatValue, shaderUniforms[getUniformNameForIndex(mSDASettings->IFB)].floatValue);
		// background color vec3 update
		shaderUniforms["iBackgroundColor"].vec3Value = vec3(shaderUniforms[getUniformNameForIndex(mSDASettings->IBR)].floatValue, shaderUniforms[getUniformNameForIndex(mSDASettings->IBG)].floatValue, shaderUniforms[getUniformNameForIndex(mSDASettings->IBB)].floatValue);
		// mouse vec4 update
		shaderUniforms["iMouse"].vec4Value = vec4(shaderUniforms[getUniformNameForIndex(mSDASettings->IMOUSEX)].floatValue, shaderUniforms[getUniformNameForIndex(mSDASettings->IMOUSEY)].floatValue, shaderUniforms[getUniformNameForIndex(mSDASettings->IMOUSEZ)].floatValue, 0.0f);
		// TODO migrate:
		if (mSDASettings->autoInvert)
		{
			setBoolUniformValueByIndex(48, (modulo < 0.1) ? 1.0 : 0.0);
		}

		if (mSDASettings->tEyePointZ)
		{
			mSDASettings->mCamEyePointZ = (modulo < 0.1) ? mSDASettings->minEyePointZ : mSDASettings->maxEyePointZ;
		}
		else
		{
			mSDASettings->mCamEyePointZ = mSDASettings->autoEyePointZ ? lmap<float>(shaderUniforms["iTempoTime"].floatValue, 0.00001, iDeltaTime, mSDASettings->minEyePointZ, mSDASettings->maxEyePointZ) : mSDASettings->mCamEyePointZ;
		}

	}
#pragma endregion animation
}

// tempo
void SDAAnimation::tapTempo()
{
	startTime = currentTime = mTimer.getSeconds();

	mTimer.stop();
	mTimer.start();

	// check for out of time values - less than 50% or more than 150% of from last "TAP and whole time buffer is going to be discarded....
	if (counter > 2 && (buffer.back() * 1.5 < currentTime || buffer.back() * 0.5 > currentTime))
	{
		buffer.clear();
		counter = 0;
		averageTime = 0;
	}
	if (counter >= 1)
	{
		buffer.push_back(currentTime);
		calculateTempo();
	}
	counter++;
}
void SDAAnimation::calculateTempo()
{
	// NORMAL AVERAGE
	double tAverage = 0;
	for (int i = 0; i < buffer.size(); i++)
	{
		tAverage += buffer[i];
	}
	averageTime = (double)(tAverage / buffer.size());
	iDeltaTime = averageTime;
	setBpm(60 / averageTime);
}
void SDAAnimation::setTimeFactor(const int &aTimeFactor)
{
	switch (aTimeFactor)
	{
	case 0:
		iTimeFactor = 0.0001;
		break;
	case 1:
		iTimeFactor = 0.125;
		break;
	case 2:
		iTimeFactor = 0.25;
		break;
	case 3:
		iTimeFactor = 0.5;
		break;
	case 4:
		iTimeFactor = 0.75;
		break;
	case 5:
		iTimeFactor = 1.0;
		break;
	case 6:
		iTimeFactor = 2.0;
		break;
	case 7:
		iTimeFactor = 4.0;
		break;
	case 8:
		iTimeFactor = 16.0;
		break;
	default:
		iTimeFactor = 1.0;
		break;
	}
}
void SDAAnimation::preventLineInCrash() {
	setUseLineIn(false);
	mSDASettings->save();
}
void SDAAnimation::saveLineIn() {
	setUseLineIn(true);
	mSDASettings->save();
}