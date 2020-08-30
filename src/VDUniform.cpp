#include "VDUniform.h"

using namespace videodromm;

VDUniform::VDUniform(VDSettingsRef aVDSettings) {
	CI_LOG_V("VDUniform ctor");
	mVDSettings = aVDSettings;
	// textures
	for (size_t i{0} ; i < 30; i++)
	{
		createSampler2DUniform("iChannel" + toString(i), 300 + i, i);// TODO verify doesn't mess up type (uint!)
	}
	createSampler2DUniform("inputImage", 399, 0);// TODO verify doesn't mess up type (uint!)

	/* TODO 20200830 mUniformsJson = getAssetPath("") / mVDSettings->mAssetsPath / "uniforms.json";
	if (fs::exists(mUniformsJson)) {
		loadUniforms(loadFile(mUniformsJson));
	}
	else {*/
		// global time in seconds
		// TODO 20200301 get rid of iTime createFloatUniform("iTime", mVDSettings->ITIME, 0.0f); // 0
		createFloatUniform("TIME", mVDSettings->ITIME, 0.0f); // 0
		// sliders
		// red
		createFloatUniform("r", mVDSettings->IFR, 0.45f); // 1
		// green
		createFloatUniform("g", mVDSettings->IFG, 0.0f); // 2
		// blue
		createFloatUniform("b", mVDSettings->IFB, 1.0f); // 3
		// Alpha 
		createFloatUniform("iAlpha", mVDSettings->IFA, 1.0f); // 4
		// red multiplier 
		createFloatUniform("iRedMultiplier", mVDSettings->IFRX, 1.0f, 0.0f, 3.0f); // 5
		// green multiplier 
		createFloatUniform("iGreenMultiplier", mVDSettings->IFGX, 1.0f, 0.0f, 3.0f); // 6
		// blue multiplier 
		createFloatUniform("iBlueMultiplier", mVDSettings->IFBX, 1.0f, 0.0f, 3.0f); // 7
		// gstnsmk
		createFloatUniform("iSobel", mVDSettings->ISOBEL, 0.02f, 0.02f, 1.0f); // 8
		// RotationSpeed
		createFloatUniform("iRotationSpeed", mVDSettings->IROTATIONSPEED, 0.02f, -0.1f, 0.1f); // 9

		// Steps
		createFloatUniform("iSteps", mVDSettings->ISTEPS, 16.0f, 1.0f, 128.0f); // 10

		// rotary
		// ratio
		//createFloatUniform("iRatio", mVDSettings->IRATIO, 1.0f, 0.01f, 1.0f); // 11
		createFloatUniform("iRatio", mVDSettings->IRATIO, 20.0f, 0.00000000001f, 20.0f); // 11
		// zoom
		createFloatUniform("iZoom", mVDSettings->IZOOM, 1.0f, 0.95f, 1.1f); // 12
		// Audio multfactor 
		createFloatUniform("iAudioMult", mVDSettings->IAUDIOX, 1.0f, 0.01f, 12.0f); // 13
		// exposure
		createFloatUniform("iExposure", mVDSettings->IEXPOSURE, 1.0f, 0.0f, 3.0f); // 14
		// Pixelate
		createFloatUniform("iPixelate", mVDSettings->IPIXELATE, 1.0f, 0.01f); // 15
		// Trixels
		createFloatUniform("iTrixels", mVDSettings->ITRIXELS, 0.0f); // 16
		// iChromatic
		createFloatUniform("iChromatic", mVDSettings->ICHROMATIC, 0.0f, 0.000000001f); // 17
		// iCrossfade
		createFloatUniform("iCrossfade", mVDSettings->IXFADE, 1.0f); // 18
		// tempo time
		createFloatUniform("iTempoTime", mVDSettings->ITEMPOTIME, 0.1f); // 19
		// fps
		createFloatUniform("iFps", mVDSettings->IFPS, 60.0f, 0.0f, 500.0f); // 20	
		// iBpm 
		createFloatUniform("iBpm", mVDSettings->IBPM, 165.0f, 0.000000001f, 400.0f); // 21
		// Speed 
		createFloatUniform("speed", mVDSettings->ISPEED, 0.01f, 0.01f, 12.0f); // 22
		// slitscan / matrix (or other) Param1 
		createFloatUniform("iPixelX", mVDSettings->IPIXELX, 0.01f, -1.5f, 1.5f); // 23
		// slitscan / matrix(or other) Param2 
		createFloatUniform("iPixelY", mVDSettings->IPIXELY, 0.01f, -1.5f, 1.5f); // 24
		// delta time in seconds
		createFloatUniform("iDeltaTime", mVDSettings->IDELTATIME, 60.0f / 160.0f); // 25

		 // background red
		createFloatUniform("iBR", mVDSettings->IBR, 0.56f); // 26
		// background green
		createFloatUniform("iBG", mVDSettings->IBG, 0.0f); // 27
		// background blue
		createFloatUniform("iBB", mVDSettings->IBB, 1.0f); // 28
		// Max Volume
		createFloatUniform("iMaxVolume", mVDSettings->IMAXVOLUME, 0.0f); // 29


		// contour
		createFloatUniform("iContour", mVDSettings->ICONTOUR, 0.0f, 0.0f, 0.5f); // 30
		// weight mix fbo texture 0
		createFloatUniform("iWeight0", mVDSettings->IWEIGHT0, 1.0f); // 31
		// weight texture 1
		createFloatUniform("iWeight1", mVDSettings->IWEIGHT1, 0.0f); // 32
		// weight texture 2
		createFloatUniform("iWeight2", mVDSettings->IWEIGHT2, 0.0f); // 33
		// weight texture 3
		createFloatUniform("iWeight3", mVDSettings->IWEIGHT3, 0.0f); // 34
		// weight texture 4
		createFloatUniform("iWeight4", mVDSettings->IWEIGHT4, 0.0f); // 35
		// weight texture 5
		createFloatUniform("iWeight5", mVDSettings->IWEIGHT5, 0.0f); // 36
		// weight texture 6
		createFloatUniform("iWeight6", mVDSettings->IWEIGHT6, 0.0f); // 37
		// weight texture 7
		createFloatUniform("iWeight7", mVDSettings->IWEIGHT7, 0.0f); // 38
		// weight texture 8 
		createFloatUniform("iWeight8", mVDSettings->IWEIGHT8, 0.0f); // 39
	


		// iMouseX  
		createFloatUniform("iMouseX", mVDSettings->IMOUSEX, 320.0f, 0.0f, 1280.0f); // 42
		// iMouseY  
		createFloatUniform("iMouseY", mVDSettings->IMOUSEY, 240.0f, 0.0f, 800.0f); // 43
		// iMouseZ  
		createFloatUniform("iMouseZ", mVDSettings->IMOUSEZ, 0.0f, 0.0f, 1.0f); // 44
		// vignette amount
		createFloatUniform("iVAmount", mVDSettings->IVAMOUNT, 0.91f, 0.0f, 1.0f); // 45
		// vignette falloff
		createFloatUniform("iVFallOff", mVDSettings->IVFALLOFF, 0.31f, 0.0f, 1.0f); // 46
		// hydra time
		//createFloatUniform("time", mVDSettings->TIME, 0.0f); // 47
		// bad tv
		createFloatUniform("iBadTv", mVDSettings->IBADTV, 0.0f, 0.0f, 5.0f); // 48
		// iTimeFactor
		createFloatUniform("iTimeFactor", mVDSettings->ITIMEFACTOR, 1.0f); // 49
		// int
		// blend mode 
		createIntUniform("iBlendmode", mVDSettings->IBLENDMODE, 0); // 50
		// beat 
		createFloatUniform("iBeat", mVDSettings->IBEAT, 0.0f, 0.0f, 300.0f); // 51
		// bar 
		createFloatUniform("iBar", mVDSettings->IBAR, 0.0f, 0.0f, 8.0f); // 52
		// bar 
		createFloatUniform("iBarBeat", mVDSettings->IBARBEAT, 1.0f, 1.0f, 1200.0f); // 53		
		// fbo A
		createIntUniform("iFboA", mVDSettings->IFBOA, 0); // 54
		// fbo B
		createIntUniform("iFboB", mVDSettings->IFBOB, 1); // 55
		// iOutW
		createIntUniform("iOutW", mVDSettings->IOUTW, mVDSettings->mRenderWidth); // 56
		// iOutH  
		createIntUniform("iOutH", mVDSettings->IOUTH, mVDSettings->mRenderHeight); // 57
		// beats per bar 
		createIntUniform("iBeatsPerBar", mVDSettings->IBEATSPERBAR, 4); // 59

		// current beat
		createFloatUniform("iPhase", mVDSettings->IPHASE, 0.0f); // 63
		// elapsed in bar 
		createFloatUniform("iElapsed", mVDSettings->IELAPSED, 0.0f); // 64
		// vec3
		// iResolutionX (should be fbowidth?) 
		createFloatUniform("iResolutionX", mVDSettings->IRESOLUTIONX, mVDSettings->mRenderWidth, 320.01f, 4280.0f); // 121
		// iResolutionY (should be fboheight?)  
		createFloatUniform("iResolutionY", mVDSettings->IRESOLUTIONY, mVDSettings->mRenderHeight, 240.01f, 2160.0f); // 122
		createVec3Uniform("iResolution", mVDSettings->IRESOLUTION, vec3(getUniformValue(mVDSettings->IRESOLUTIONX), getUniformValue(mVDSettings->IRESOLUTIONY), 1.0)); // 120


		createVec3Uniform("iColor", mVDSettings->ICOLOR, vec3(0.45, 0.0, 1.0)); // 61
		createVec3Uniform("iBackgroundColor", mVDSettings->IBACKGROUNDCOLOR); // 62
		//createVec3Uniform("iChannelResolution[0]", 63, vec3(mVDParams->getFboWidth(), mVDParams->getFboHeight(), 1.0));

		// vec4
		createVec4Uniform("iMouse", mVDSettings->IMOUSE, vec4(320.0f, 240.0f, 0.0f, 0.0f));//70
		createVec4Uniform("iDate", mVDSettings->IDATE, vec4(2019.0f, 12.0f, 1.0f, 5.0f));//71

		// boolean
		// invert
		// glitch
		createBoolUniform("iGlitch", mVDSettings->IGLITCH); // 81
		// vignette
		createBoolUniform("iVignette", mVDSettings->IVIGN); // 82 toggle
		// toggle
		createBoolUniform("iToggle", mVDSettings->ITOGGLE); // 83
		// invert
		createBoolUniform("iInvert", mVDSettings->IINVERT); // 86
		// greyscale 
		createBoolUniform("iGreyScale", mVDSettings->IGREYSCALE); //87
		createBoolUniform("iClear", mVDSettings->ICLEAR, true); // 88
		createBoolUniform("iDebug", mVDSettings->IDEBUG); // 129
		createBoolUniform("iXorY", mVDSettings->IXORY); // 130
		createBoolUniform("iFlipH", mVDSettings->IFLIPH); // 131
		createBoolUniform("iFlipV", mVDSettings->IFLIPV); // 132
		createBoolUniform("iFlipPostH", mVDSettings->IFLIPPOSTH); // 133
		createBoolUniform("iFlipPostV", mVDSettings->IFLIPPOSTV); // 134

		// 119 to 124 timefactor from midithor sos
		// floats for warps
		// srcArea 
		createFloatUniform("srcXLeft", mVDSettings->SRCXLEFT, 0.0f, 0.0f, 4280.0f); // 160
		createFloatUniform("srcXRight", mVDSettings->SRCXRIGHT, mVDSettings->mRenderWidth, 320.01f, 4280.0f); // 161
		createFloatUniform("srcYLeft", mVDSettings->SRCYLEFT, 0.0f, 0.0f, 1024.0f); // 162
		createFloatUniform("srcYRight", mVDSettings->SRCYRIGHT, mVDSettings->mRenderHeight, 0.0f, 1024.0f); // 163
		// iFreq0  
		createFloatUniform("iFreq0", mVDSettings->IFREQ0, 0.0f, 0.01f, 256.0f); // 140	
		// iFreq1  
		createFloatUniform("iFreq1", mVDSettings->IFREQ1, 0.0f, 0.01f, 256.0f); // 141
		// iFreq2  
		createFloatUniform("iFreq2", mVDSettings->IFREQ2, 0.0f, 0.01f, 256.0f); // 142
		// iFreq3  
		createFloatUniform("iFreq3", mVDSettings->IFREQ3, 0.0f, 0.01f, 256.0f); // 143

		// vec2
		createVec2Uniform("resolution", mVDSettings->RESOLUTION, vec2(1280.0f, 720.0f)); // hydra 150
		createVec2Uniform("RENDERSIZE", mVDSettings->RENDERSIZE, vec2(getUniformValueByName("iResolutionX"), getUniformValueByName("iResolutionY"))); // isf 151

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
	//}
}
void VDUniform::loadUniforms(const ci::DataSourceRef& source) {

	JsonTree json(source);

	// try to load the specified json file
	if (json.hasChild("uniforms")) {
		JsonTree u(json.getChild("uniforms"));

		// iterate uniforms
		for (size_t i{0} ; i < u.getNumChildren(); i++) {
			JsonTree child(u.getChild(i));

			if (child.hasChild("uniform")) {
				JsonTree w(child.getChild("uniform"));
				// create uniform of the correct type
				int uniformType = (w.hasChild("type")) ? w.getValueForKey<int>("type") : 0;
				switch (uniformType) {
				case GL_FLOAT:
					// float 5126 GL_FLOAT 0x1406
					floatFromJson(child);
					break;
				case GL_SAMPLER_2D:
					// sampler2d 35678 GL_SAMPLER_2D 0x8B5E
					sampler2dFromJson(child);
					break;
				case GL_FLOAT_VEC2:
					// vec2 35664
					vec2FromJson(child);
					break;
				case GL_FLOAT_VEC3:
					// vec3 35665
					vec3FromJson(child);
					break;
				case GL_FLOAT_VEC4:
					// vec4 35666 GL_FLOAT_VEC4
					vec4FromJson(child);
					break;
				case GL_INT:
					// int 5124 GL_INT 0x1404
					intFromJson(child);
					break;
				case GL_BOOL:
					// boolean 35670 GL_BOOL 0x8B56
					boolFromJson(child);
					break;
				}
			}
		}
	}
}

void VDUniform::floatFromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex;
	float jValue, jMin, jMax;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 249;
		jValue = (u.hasChild("value")) ? u.getValueForKey<float>("value") : 0.01f;
		jMin = (u.hasChild("min")) ? u.getValueForKey<float>("min") : 0.0f;
		jMax = (u.hasChild("max")) ? u.getValueForKey<float>("max") : 1.0f;
		createFloatUniform(jName, jCtrlIndex, jValue, jMin, jMax);
	}
}
void VDUniform::sampler2dFromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex;
	int jTextureIndex;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 250;
		jTextureIndex = (u.hasChild("textureindex")) ? u.getValueForKey<int>("textureindex") : 0;;
		createSampler2DUniform(jName, jTextureIndex);
	}
}
void VDUniform::vec2FromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 251;
		createVec2Uniform(jName, jCtrlIndex);
	}
}
void VDUniform::vec3FromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 252;
		createVec3Uniform(jName, jCtrlIndex);
	}
}
void VDUniform::vec4FromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 253;
		createVec4Uniform(jName, jCtrlIndex);
	}
}
void VDUniform::intFromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex, jValue;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 254;
		jValue = (u.hasChild("value")) ? u.getValueForKey<int>("value") : 1;
		createIntUniform(jName, jCtrlIndex, jValue);
	}

}
void VDUniform::boolFromJson(const ci::JsonTree& json) {
	std::string jName;
	int jCtrlIndex;
	bool jValue;
	if (json.hasChild("uniform")) {
		JsonTree u(json.getChild("uniform"));
		jName = (u.hasChild("name")) ? u.getValueForKey<std::string>("name") : "unknown";
		jCtrlIndex = (u.hasChild("index")) ? u.getValueForKey<int>("index") : 255;
		jValue = (u.hasChild("value")) ? u.getValueForKey<bool>("value") : false;
		createBoolUniform(jName, jCtrlIndex, jValue);
	}
}
