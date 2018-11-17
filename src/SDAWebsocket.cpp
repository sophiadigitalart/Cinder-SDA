#include "SDAWebsocket.h"

using namespace SophiaDigitalArt;

SDAWebsocket::SDAWebsocket(SDASettingsRef aSDASettings, SDAAnimationRef aSDAAnimation) {
	mSDASettings = aSDASettings;
	mSDAAnimation = aSDAAnimation;

	CI_LOG_V("SDAWebsocket constructor");
	shaderReceived = false;
	receivedFragString = "";
	streamReceived = false;
	// WebSockets
	clientConnected = false;

	if (mSDASettings->mAreWebSocketsEnabledAtStartup) wsConnect();
	mPingTime = getElapsedSeconds();

}

void SDAWebsocket::updateParams(int iarg0, float farg1) {
	if (farg1 > 0.1) {
		//avoid to send twice
		if (iarg0 == 61) {
			// right arrow
			mSDASettings->iBlendmode--;
			if (mSDASettings->iBlendmode < 0) mSDASettings->iBlendmode = mSDAAnimation->getBlendModesCount() - 1;
		}
		if (iarg0 == 62) {
			// left arrow
			mSDASettings->iBlendmode++;
			if (mSDASettings->iBlendmode > mSDAAnimation->getBlendModesCount() - 1) mSDASettings->iBlendmode = 0;
		}
	}
	if (iarg0 > 0 && iarg0 < 9) {
		// sliders 
		mSDAAnimation->setFloatUniformValueByIndex(iarg0, farg1);
	}
	if (iarg0 > 10 && iarg0 < 19) {
		// rotary 
		mSDAAnimation->setFloatUniformValueByIndex(iarg0, farg1);
		// audio multfactor
		if (iarg0 == 13) mSDAAnimation->setFloatUniformValueByIndex(iarg0, (farg1 + 0.01) * 10);
		// exposure
		if (iarg0 == 14) mSDAAnimation->setFloatUniformValueByIndex(iarg0, (farg1 + 0.01) * mSDAAnimation->getMaxUniformValueByIndex(14));

		wsWrite("{\"params\" :[{\"name\":" + toString(iarg0) + ",\"value\":" + toString(mSDAAnimation->getFloatUniformValueByIndex(iarg0)) + "}]}");

	}
	// buttons
	if (iarg0 > 20 && iarg0 < 29) {
		// select index
		mSDASettings->selectedWarp = iarg0 - 21;
	}
	/*if (iarg0 > 30 && iarg0 < 39)
	{
	// select input
	mSDASettings->mWarpFbos[mSDASettings->selectedWarp].textureIndex = iarg0 - 31;
	// activate
	mSDASettings->mWarpFbos[mSDASettings->selectedWarp].active = !mSDASettings->mWarpFbos[mSDASettings->selectedWarp].active;
	}*/
	if (iarg0 > 40 && iarg0 < 49) {
		// low row 
		mSDAAnimation->setFloatUniformValueByIndex(iarg0, farg1);
	}
}

void SDAWebsocket::wsPing() {
#if defined( CINDER_MSW )
	if (clientConnected) {
		if (!mSDASettings->mIsWebSocketsServer) {
			mClient.ping();
		}
	}
#endif
}
string * SDAWebsocket::getBase64Image() { 
	streamReceived = false; 
	return &mBase64String; 
}

void SDAWebsocket::parseMessage(string msg) {
	mSDASettings->mWebSocketsMsg = "WS onRead";
	mSDASettings->mWebSocketsNewMsg = true;
	if (!msg.empty()) {
		mSDASettings->mWebSocketsMsg += ": " + msg;
		CI_LOG_V("ws msg: " + msg);
		string first = msg.substr(0, 1);
		if (first == "{") {
			// json
			JsonTree json;
			try {
				json = JsonTree(msg);
				// web controller
				if (json.hasChild("params")) {
					JsonTree jsonParams = json.getChild("params");
					for (JsonTree::ConstIter jsonElement = jsonParams.begin(); jsonElement != jsonParams.end(); ++jsonElement) {
						int name = jsonElement->getChild("name").getValue<int>();
						float value = jsonElement->getChild("value").getValue<float>();
						// basic name value 
						mSDAAnimation->setFloatUniformValueByIndex(name, value);
					}
				}

				if (json.hasChild("k2")) {
					JsonTree jsonParams = json.getChild("k2");
					for (JsonTree::ConstIter jsonElement = jsonParams.begin(); jsonElement != jsonParams.end(); ++jsonElement) {
						int name = jsonElement->getChild("name").getValue<int>();
						string value = jsonElement->getChild("value").getValue();
						vector<string> vs = split(value, ",");
						vec4 v = vec4(strtof((vs[0]).c_str(), 0), strtof((vs[1]).c_str(), 0), strtof((vs[2]).c_str(), 0), strtof((vs[3]).c_str(), 0));
						// basic name value 
						mSDAAnimation->setVec4UniformValueByIndex(name, v);
					}
				}

				if (json.hasChild("event")) {
					JsonTree jsonEvent = json.getChild("event");
					string val = jsonEvent.getValue();
					// check if message exists
					if (json.hasChild("message")) {
						if (val == "canvas") {
							// we received a jpeg base64
							mBase64String = json.getChild("message").getValue<string>();
							streamReceived = true;
						}
						else if (val == "params") {
							//{"event":"params","message":"{\"params\" :[{\"name\" : 12,\"value\" :0.132}]}"}
							JsonTree jsonParams = json.getChild("message");
							for (JsonTree::ConstIter jsonElement = jsonParams.begin(); jsonElement != jsonParams.end(); ++jsonElement) {
								int name = jsonElement->getChild("name").getValue<int>();
								float value = jsonElement->getChild("value").getValue<float>();
								CI_LOG_V("SDAWebsocket jsonParams.mValue:" + toString(name) );
								// basic name value 
								mSDAAnimation->setFloatUniformValueByIndex(name, value);
							}
						}
						else {
							// we received a fragment shader string
							receivedFragString = json.getChild("message").getValue<string>();
							shaderReceived = true;
						}
						//string evt = json.getChild("event").getValue<string>();
					}
				}
				if (json.hasChild("cmd")) {
					JsonTree jsonCmd = json.getChild("cmd");
					for (JsonTree::ConstIter jsonElement = jsonCmd.begin(); jsonElement != jsonCmd.end(); ++jsonElement) {
						receivedType = jsonElement->getChild("type").getValue<int>();
						switch (receivedType)
						{
						case 2:
							// change tempo
							mSDAAnimation->setBpm(jsonElement->getChild("tempo").getValue<float>());
							break;
						default:
							break;
						}

					}

				}
			}
			catch (cinder::JsonTree::Exception exception) {
				mSDASettings->mWebSocketsMsg += " error jsonparser exception: ";
				mSDASettings->mWebSocketsMsg += exception.what();
				mSDASettings->mWebSocketsMsg += "  ";
			}
		}
		else if (msg.substr(0, 2) == "/*") {
			// shader with json info				
			unsigned closingCommentPosition = msg.find("*/");
			if (closingCommentPosition > 0) {
				JsonTree json;
				try {
					// create folders if they don't exist
					fs::path pathsToCheck = getAssetPath("") / "glsl";
					if (!fs::exists(pathsToCheck)) fs::create_directory(pathsToCheck);
					pathsToCheck = getAssetPath("") / "glsl" / "received";
					if (!fs::exists(pathsToCheck)) fs::create_directory(pathsToCheck);
					pathsToCheck = getAssetPath("") / "glsl" / "processed";
					if (!fs::exists(pathsToCheck)) fs::create_directory(pathsToCheck);
					// find commented header
					string jsonHeader = msg.substr(2, closingCommentPosition - 2);
					ci::JsonTree::ParseOptions parseOptions;
					parseOptions.ignoreErrors(false);
					json = JsonTree(jsonHeader, parseOptions);
					string title = json.getChild("title").getValue<string>();
					string fragFileName = title + ".frag"; // with uniforms
					string glslFileName = title + ".glsl"; // without uniforms, need to include shadertoy.inc
					string shader = msg.substr(closingCommentPosition + 2);

					string processedContent = "/*" + jsonHeader + "*/";
					// check uniforms presence
					std::size_t foundUniform = msg.find("uniform");

					if (foundUniform != std::string::npos) {
						// found uniforms
					}
					else {
						// save glsl file without uniforms as it was received
						fs::path currentFile = getAssetPath("") / "glsl" / "received" / glslFileName;
						ofstream mFrag(currentFile.string(), std::ofstream::binary);
						mFrag << msg;
						mFrag.close();
						CI_LOG_V("received file saved:" + currentFile.string());
						//mSDASettings->mShaderToLoad = currentFile.string(); 
						// uniforms not found, add include
						processedContent += "#include shadertoy.inc";
					}
					processedContent += shader;

					//mShaders->loadLiveShader(processedContent); // need uniforms declared
					// route it to websockets clients
					if (mSDASettings->mIsRouter) {
						wsWrite(msg);
					}

					// save processed file
					fs::path processedFile = getAssetPath("") / "glsl" / "processed" / fragFileName;
					ofstream mFragProcessed(processedFile.string(), std::ofstream::binary);
					mFragProcessed << processedContent;
					mFragProcessed.close();
					CI_LOG_V("processed file saved:" + processedFile.string());
					// USELESS? mSDASettings->mShaderToLoad = processedFile.string();
				}
				catch (cinder::JsonTree::Exception exception) {
					mSDASettings->mWebSocketsMsg += " error jsonparser exception: ";
					mSDASettings->mWebSocketsMsg += exception.what();
					mSDASettings->mWebSocketsMsg += "  ";
				}
			}
		}
		/* OBSOLETE
		else if (msg.substr(0, 7) == "uniform") {
			// fragment shader from live coding
			mSDASettings->mShaderToLoad = msg;
			// route it to websockets clients
			if (mSDASettings->mIsRouter) {
				wsWrite(msg);
			}
		}*/
		else if (msg.substr(0, 7) == "#version") {
			// fragment shader from live coding
			//mShaders->loadLiveShader(msg);
			// route it to websockets clients
			if (mSDASettings->mIsRouter) {
				wsWrite(msg);
			}
		}
		else if (first == "/")
		{
			// osc from videodromm-nodejs-router
			/*int f = msg.size();
			const char c = msg[9];
			int s = msg[12];
			int t = msg[13];
			int u = msg[14];*/
			CI_LOG_V(msg);
		}
		else if (first == "I") {

			if (msg == "ImInit") {
				// send ImInit OK
				/*if (!remoteClientActive) { remoteClientActive = true; ForceKeyFrame = true;
				// Send confirmation mServer.write("ImInit"); // Send font texture unsigned char* pixels; int width, height;
				ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&pixels, &width, &height); PreparePacketTexFont(pixels, width, height);SendPacket();
				}*/
			}
			else if (msg.substr(0, 11) == "ImMouseMove") {
				/*string trail = msg.substr(12);
				unsigned commaPosition = trail.find(",");
				if (commaPosition > 0) { mouseX = atoi(trail.substr(0, commaPosition).c_str());
				mouseY = atoi(trail.substr(commaPosition + 1).c_str()); ImGuiIO& io = ImGui::GetIO();
				io.MousePos = toPixels(vec2(mouseX, mouseY)); }*/
			}
			else if (msg.substr(0, 12) == "ImMousePress") {
				/*ImGuiIO& io = ImGui::GetIO(); // 1,0 left click 1,1 right click
				io.MouseDown[0] = false; io.MouseDown[1] = false; int rightClick = atoi(msg.substr(15).c_str());
				if (rightClick == 1) { io.MouseDown[0] = false; io.MouseDown[1] = true; }
				else { io.MouseDown[0] = true; io.MouseDown[1] = false;
				}*/
			}
		}
	}
}
string SDAWebsocket::getReceivedShader() {
	shaderReceived = false;
	return receivedFragString;
}
void SDAWebsocket::wsConnect() {

	// either a client or a server
	if (mSDASettings->mIsWebSocketsServer) {
		mServer.connectOpenEventHandler([&]() {
			clientConnected = true;
			mSDASettings->mWebSocketsMsg = "Connected";
			mSDASettings->mWebSocketsNewMsg = true;
		});
		mServer.connectCloseEventHandler([&]() {
			clientConnected = false;
			mSDASettings->mWebSocketsMsg = "Disconnected";
			mSDASettings->mWebSocketsNewMsg = true;
		});
		mServer.connectFailEventHandler([&](string err) {
			mSDASettings->mWebSocketsMsg = "WS Error";
			mSDASettings->mWebSocketsNewMsg = true;
			if (!err.empty()) {
				mSDASettings->mWebSocketsMsg += ": " + err;
			}
		});
		mServer.connectInterruptEventHandler([&]() {
			mSDASettings->mWebSocketsMsg = "WS Interrupted";
			mSDASettings->mWebSocketsNewMsg = true;
		});
		mServer.connectPingEventHandler([&](string msg) {
			mSDASettings->mWebSocketsMsg = "WS Pinged";
			mSDASettings->mWebSocketsNewMsg = true;
			if (!msg.empty())
			{
				mSDASettings->mWebSocketsMsg += ": " + msg;
			}
		});
		mServer.connectMessageEventHandler([&](string msg) {
			parseMessage(msg);
		});
		mServer.listen(mSDASettings->mWebSocketsPort);
	}
	else
	{
		mClient.connectOpenEventHandler([&]() {
			clientConnected = true;
			mSDASettings->mWebSocketsMsg = "Connected";
			mSDASettings->mWebSocketsNewMsg = true;
		});
		mClient.connectCloseEventHandler([&]() {
			clientConnected = false;
			mSDASettings->mWebSocketsMsg = "Disconnected";
			mSDASettings->mWebSocketsNewMsg = true;
		});
		mClient.connectFailEventHandler([&](string err) {
			mSDASettings->mWebSocketsMsg = "WS Error";
			mSDASettings->mWebSocketsNewMsg = true;
			if (!err.empty()) {
				mSDASettings->mWebSocketsMsg += ": " + err;
			}
		});
		mClient.connectInterruptEventHandler([&]() {
			mSDASettings->mWebSocketsMsg = "WS Interrupted";
			mSDASettings->mWebSocketsNewMsg = true;
		});
		mClient.connectPingEventHandler([&](string msg) {
			mSDASettings->mWebSocketsMsg = "WS Ponged";
			mSDASettings->mWebSocketsNewMsg = true;
			if (!msg.empty())
			{
				mSDASettings->mWebSocketsMsg += ": " + msg;
			}
		});
		mClient.connectMessageEventHandler([&](string msg) {
			parseMessage(msg);
		});
		wsClientConnect();
	}
	mSDASettings->mAreWebSocketsEnabledAtStartup = true;
	clientConnected = true;

}
void SDAWebsocket::wsClientConnect()
{

	stringstream s;
	s << mSDASettings->mWebSocketsProtocol << mSDASettings->mWebSocketsHost << ":" << mSDASettings->mWebSocketsPort;
	mClient.connect(s.str());

}
void SDAWebsocket::wsClientDisconnect()
{

	if (clientConnected)
	{
		mClient.disconnect();
	}

}
void SDAWebsocket::wsWrite(string msg)
{

	if (mSDASettings->mAreWebSocketsEnabledAtStartup)
	{
		if (mSDASettings->mIsWebSocketsServer)
		{
			mServer.write(msg);
		}
		else
		{
			if (clientConnected) mClient.write(msg);
		}
	}

}

void SDAWebsocket::sendJSON(string params) {

	wsWrite(params);

}
void SDAWebsocket::toggleAuto(unsigned int aIndex) {
	// toggle
	mSDAAnimation->toggleAuto(aIndex);
	// TODO send json	
}
void SDAWebsocket::toggleTempo(unsigned int aIndex) {
	// toggle
	mSDAAnimation->toggleTempo(aIndex);
	// TODO send json	
}
void SDAWebsocket::toggleValue(unsigned int aIndex) {
	// toggle
	mSDAAnimation->toggleValue(aIndex);
	stringstream sParams;
	// TODO check boolean value:
	sParams << "{\"params\" :[{\"name\" : " << aIndex << ",\"value\" : " << (int)mSDAAnimation->getBoolUniformValueByIndex(aIndex) << "}]}";
	string strParams = sParams.str();
	sendJSON(strParams);
}
void SDAWebsocket::resetAutoAnimation(unsigned int aIndex) {
	// reset
	mSDAAnimation->resetAutoAnimation(aIndex);
	// TODO: send json	
}

void SDAWebsocket::changeBoolValue(unsigned int aControl, bool aValue) {
	// check if changed
	mSDAAnimation->setBoolUniformValueByIndex(aControl, aValue);
	stringstream sParams;
	// TODO: check boolean value:
	sParams << "{\"params\" :[{\"name\" : " << aControl << ",\"value\" : " << (int)aValue << "}]}";
	string strParams = sParams.str();
	sendJSON(strParams);
}

void SDAWebsocket::changeFloatValue(unsigned int aControl, float aValue, bool forceSend) {
	/*if (aControl == 31) {
		CI_LOG_V("old value " + toString(mSDAAnimation->getFloatUniformValueByIndex(aControl)) + " newvalue " + toString(aValue));
	}*/
	// check if changed
	if ( (mSDAAnimation->setFloatUniformValueByIndex(aControl, aValue) && aControl != mSDASettings->IFPS) || forceSend) {
		stringstream sParams;
		// update color vec3
		if (aControl > 0 && aControl < 4) {
			mSDAAnimation->setVec3UniformValueByIndex(61, vec3(mSDAAnimation->getFloatUniformValueByIndex(1), mSDAAnimation->getFloatUniformValueByIndex(2), mSDAAnimation->getFloatUniformValueByIndex(3)));
			colorWrite(); //lights4events
		}
		if (aControl == 29 || aControl ==30) {
			mSDAAnimation->setVec3UniformValueByIndex(60, vec3(mSDAAnimation->getFloatUniformValueByIndex(29), mSDAAnimation->getFloatUniformValueByIndex(30), 1.0));
		}
		sParams << "{\"params\" :[{\"name\" : " << aControl << ",\"value\" : " << mSDAAnimation->getFloatUniformValueByIndex(aControl) << "}]}";
		string strParams = sParams.str();
		sendJSON(strParams);
	}
}
void SDAWebsocket::changeShaderIndex(unsigned int aWarpIndex, unsigned int aWarpShaderIndex, unsigned int aSlot) {
	//aSlot 0 = A, 1 = B,...
	stringstream sParams;
	sParams << "{\"cmd\" :[{\"type\" : 1,\"warp\" : " << aWarpIndex << ",\"shader\" : " << aWarpShaderIndex << ",\"slot\" : " << aSlot << "}]}";
	string strParams = sParams.str();
	sendJSON(strParams);
}
void SDAWebsocket::changeWarpFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex, unsigned int aSlot) {
	//aSlot 0 = A, 1 = B,...
	stringstream sParams;
	sParams << "{\"cmd\" :[{\"type\" : 0,\"warp\" : " << aWarpIndex << ",\"fbo\" : " << aWarpFboIndex << ",\"slot\" : " << aSlot << "}]}";
	string strParams = sParams.str();
	sendJSON(strParams);
}
void SDAWebsocket::changeFragmentShader(string aFragmentShaderText) {

	stringstream sParams;
	sParams << "{\"event\" : \"frag\",\"message\" : \"" << aFragmentShaderText << "\"}";
	string strParams = sParams.str();
	sendJSON(strParams);
}
void SDAWebsocket::colorWrite()
{

	// lights4events
	char col[8];
	int r = (int)(mSDAAnimation->getFloatUniformValueByIndex(1) * 255);
	int g = (int)(mSDAAnimation->getFloatUniformValueByIndex(2) * 255);
	int b = (int)(mSDAAnimation->getFloatUniformValueByIndex(3) * 255);
	sprintf(col, "#%02X%02X%02X", r, g, b);
	wsWrite(col);

}

void SDAWebsocket::update() {

	// websockets

	if (mSDASettings->mAreWebSocketsEnabledAtStartup)
	{
		if (mSDASettings->mIsWebSocketsServer)
		{
			mServer.poll();
		}
		else
		{
			if (clientConnected)
			{
				mClient.poll();
			}
		}
	}

	/* OLD KINECT AND TOUCHOSC
	// check for mouse moved message
	if(m.getAddress() == "/mouse/position"){
	// both the arguments are int32's
	Vec2i pos = Vec2i( m.getArgAsInt32(0), m.getArgAsInt32(1));
	Vec2f mouseNorm = Vec2f( pos ) / getWindowSize();
	Vec2f mouseVel = Vec2f( pos - pMouse ) / getWindowSize();
	addToFluid( mouseNorm, mouseVel, true, true );
	pMouse = pos;
	if ( m.getArgAsInt32(2) == 1 )
	{
	mMouseDown = true;
	}
	else
	{
	mMouseDown = false;
	}
	if ( mMouseDown )
	{
	mArcball.mouseDown( pos );
	mCurrentMouseDown = mInitialMouseDown = pos;
	}
	}
	// check for mouse button message
	else if(m.getAddress() == "/mouse/button"){
	// the single argument is a string
	Vec2i pos = Vec2i( m.getArgAsInt32(0), m.getArgAsInt32(1));
	mArcball.mouseDown( pos );
	mCurrentMouseDown = mInitialMouseDown = pos;
	if ( m.getArgAsInt32(2) == 1 )
	{
	mMouseDown = true;
	}
	else
	{
	mMouseDown = false;
	}
	}
	else if(m.getAddress() == "/fluid/drawfluid"){
	drawFluid = !drawFluid;
	}
	else if(m.getAddress() == "/fluid/drawfluidtex"){
	drawFluidTex = !drawFluidTex;
	}
	else if(m.getAddress() == "/fluid/drawparticles"){
	drawParticles = ! drawParticles;
	}
	else if(m.getAddress() == "/fluid/randomizecolor"){
	fluidSolver.randomizeColor();
	}
	else if(m.getAddress() == "/window/position"){
	// window position
	setWindowPos(m.getArgAsInt32(0), m.getArgAsInt32(1));
	}
	else if(m.getAddress() == "/window/setfullscreen"){
	// fullscreen
	//setFullScreen( ! isFullScreen() );
	}
	else if(m.getAddress() == "/quit"){
	quitProgram();
	}
	else{
	// unrecognized message
	//cout << "not recognized:" << m.getAddress() << endl;

	}

	}
	// osc
	while (mOSCReceiver.hasWaitingMessages())
	{
	osc::Message message;
	bool routeMessage = false;
	mOSCReceiver.getNextMessage(&message);
	for (int a = 0; a < MAX; a++)
	{
	iargs[a] = 0;
	fargs[a] = 0.0;
	sargs[a] = "";
	}
	string oscAddress = message.getAddress();

	int numArgs = message.getNumArgs();
	// get arguments
	for (int i = 0; i < message.getNumArgs(); i++)
	{
	if (i < MAX)
	{
	if (message.getArgType(i) == osc::TYPE_INT32) {
	try
	{
	iargs[i] = message.getArgAsInt32(i);
	sargs[i] = toString(iargs[i]);
	}
	catch (...) {
	cout << "Exception reading argument as int32" << std::endl;
	}
	}
	if (message.getArgType(i) == osc::TYPE_FLOAT) {
	try
	{
	fargs[i] = message.getArgAsFloat(i);
	sargs[i] = toString(fargs[i]);
	}
	catch (...) {
	cout << "Exception reading argument as float" << std::endl;
	}
	}
	if (message.getArgType(i) == osc::TYPE_STRING) {
	try
	{
	sargs[i] = message.getArgAsString(i);
	}
	catch (...) {
	cout << "Exception reading argument as string" << std::endl;
	}
	}
	}
	}



	{
	console() << "OSC message received: " << oscAddress << std::endl;
	// is it a layer msg?
	int layer = 0;
	unsigned layerFound = oscAddress.find("layer");
	if (layerFound == 1)
	{
	unsigned clipFound = oscAddress.find("/clip");
	if (clipFound == 7) // layer must be < 10
	{
	cout << "clipFound " << clipFound;
	layer = atoi(oscAddress.substr(6, 1).c_str());
	int clip = atoi(oscAddress.substr(12, 1).c_str());
	string fileName = toString((layer * 10) + clip) + ".fragjson";
	fs::path fragFile = getAssetPath("") / "shaders" / "fragjson" / fileName;
	if (fs::exists(fragFile))
	{
	//mShaders->loadFragJson(fragFile.string());
	}
	}
	else
	{
	if (clipFound == 8)
	{
	layer = atoi(oscAddress.substr(6, 2).c_str());
	}
	}
	// connect or preview
	unsigned connectFound = oscAddress.find("connect");
	if (connectFound != string::npos) cout << "connectFound " << connectFound;
	}
	//if ( layerFound != string::npos ) cout << "layerFound " << layerFound;

	unsigned found = oscAddress.find_last_of("/");
	int name = atoi(oscAddress.substr(found + 1).c_str());
	}
	stringstream ss;
	ss << message.getRemoteIp() << " adr:" << oscAddress << " ";
	for (int a = 0; a < MAX; a++)
	{
	ss << a << ":" << sargs[a] << " ";
	}
	ss << std::endl;
	mSDASettings->mWebSocketsNewMsg = true;
	mSDASettings->mWebSocketsMsg = ss.str();
	*/
}
