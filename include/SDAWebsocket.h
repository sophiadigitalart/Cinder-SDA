#pragma once
#include "cinder/Cinder.h"
#include "cinder/app/App.h"
#include "cinder/Json.h"

// Settings
#include "SDASettings.h"
// Animation
#include "SDAAnimation.h"

// WebSockets
#include "WebSocketClient.h"
#include "WebSocketServer.h"


using namespace ci;
using namespace ci::app;
using namespace std;
using namespace asio;
//using namespace asio::ip; 

namespace SophiaDigitalArt
{
	// stores the pointer to the SDAWebsocket instance
	typedef std::shared_ptr<class SDAWebsocket> SDAWebsocketRef;
	class SDAWebsocket {
	public:
		SDAWebsocket(SDASettingsRef aSDASettings, SDAAnimationRef aSDAAnimation);
		static SDAWebsocketRef	create(SDASettingsRef aSDASettings, SDAAnimationRef aSDAAnimation)
		{
			return shared_ptr<SDAWebsocket>(new SDAWebsocket(aSDASettings, aSDAAnimation));
		}
		void						update();
        // messages
		void						sendJSON(string params);
		void						updateParams(int iarg0, float farg1);
		// WebSockets
		//void						wsWriteBinary(const void *data, int size);
		void						wsWrite(std::string msg);
		void						wsConnect();
		void						wsPing();
		//bool						isWsClientConnected() { return clientConnected; };
		// change a control value and update network clients
		void						changeFloatValue(unsigned int aControl, float aValue, bool forceSend = false);
		void						changeBoolValue(unsigned int aControl, bool aValue);
		void						toggleAuto(unsigned int aIndex);
		void						toggleValue(unsigned int aIndex);
		void						toggleTempo(unsigned int aIndex);
		void						resetAutoAnimation(unsigned int aIndex);
		void						changeShaderIndex(unsigned int aWarpIndex, unsigned int aWarpShaderIndex, unsigned int aSlot);
		void						changeWarpFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex, unsigned int aSlot); //aSlot 0 = A, 1 = B,...
		void                        changeFragmentShader(string aFragmentShaderText);
		// received shaders
		bool						hasReceivedShader() { return shaderReceived; };
		string						getReceivedShader();
		// received stream
		string *					getBase64Image();
		bool						hasReceivedStream() { return streamReceived; };
	private:
		// Settings
		SDASettingsRef				mSDASettings;
		// Animation
		SDAAnimationRef				mSDAAnimation;

		// lights4events
		void						colorWrite();
		// WebSockets
		void						parseMessage(string msg);
		// Web socket client
		bool						clientConnected;
		void						wsClientConnect();
		void						wsClientDisconnect();
		int							receivedType;
		int							receivedWarpIndex;
		int							receivedFboIndex;
		int							receivedShaderIndex;
		int							receivedSlot;

		WebSocketClient				mClient;
		void						onWsConnect();
		void						onWsDisconnect();
		void						onWsError(std::string err);
		void						onWsInterrupt();
		void						onWsPing(std::string msg);
		void						onWsRead(std::string msg);
		// Web socket  server
		WebSocketServer				mServer;
		void						serverConnect();
		void						serverDisconnect();
		double						mPingTime;

		// received shaders
		bool						shaderReceived; // TODO remove
		string						receivedFragString; // TODO remove
		string						mBase64String;
		// received stream
		bool						streamReceived;
	};
}

