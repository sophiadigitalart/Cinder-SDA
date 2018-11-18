#pragma once
#include "cinder/Cinder.h"
#include "cinder/app/App.h"
#include "cinder/Json.h"

// Settings
#include "SDASettings.h"
// Animation
#include "SDAAnimation.h"
// Websocket
#include "SDAWebsocket.h"
// Midi
#include "MidiIn.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace asio;
using namespace asio::ip; 
using namespace SophiaDigitalArt;

namespace SophiaDigitalArt
{
	// stores the pointer to the SDARouter instance
	typedef std::shared_ptr<class SDARouter> SDARouterRef;
	// midi
	typedef std::shared_ptr<class MIDI> MIDIRef;

	struct midiInput
	{
		string			portName;
		bool			isConnected;
	};
	struct midiOutput
	{
		string			portName;
		bool			isConnected;
	};

	class SDARouter {
	public:
		SDARouter(SDASettingsRef aSDASettings, SDAAnimationRef aSDAAnimation, SDAWebsocketRef aSDAWebsocket);
		static SDARouterRef	create(SDASettingsRef aSDASettings, SDAAnimationRef aSDAAnimation, SDAWebsocketRef aSDAWebsocket)
		{
			return shared_ptr<SDARouter>(new SDARouter(aSDASettings, aSDAAnimation, aSDAWebsocket));
		}
		//void						update();
		void						shutdown();
		// messages
		void						updateParams(int iarg0, float farg1);
		// MIDI
		void						midiSetup();
		int							getMidiInPortsCount() { return mMidiInputs.size(); };
		string						getMidiInPortName(unsigned int i) { return (i < mMidiInputs.size()) ? mMidiInputs[i].portName : "No midi in ports"; };
		bool						isMidiInConnected(unsigned int i) { return (i < mMidiInputs.size()) ? mMidiInputs[i].isConnected : false; };
		int							getMidiOutPortsCount() { return mMidiOutputs.size(); };
		string						getMidiOutPortName(unsigned int i) { return (i < mMidiOutputs.size()) ? mMidiOutputs[i].portName : "No midi out ports"; };
		bool						isMidiOutConnected(unsigned int i) { return (i < mMidiOutputs.size()) ? mMidiOutputs[i].isConnected : false; };
		void						midiOutSendNoteOn(int i, int channel, int pitch, int velocity);

		void						openMidiInPort(int i);
		void						closeMidiInPort(int i);
		void						openMidiOutPort(int i);
		void						closeMidiOutPort(int i);

		int							selectedWarp() { return mSelectedWarp; };
		int							selectedFboA() { return mSelectedFboA; };
		int							selectedFboB() { return mSelectedFboB; };
		void						setWarpAFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex) { mSelectedFboA = aWarpFboIndex; }
		void						setWarpBFboIndex(unsigned int aWarpIndex, unsigned int aWarpFboIndex) { mSelectedFboB = aWarpFboIndex; }
	private:
		// Settings
		SDASettingsRef				mSDASettings;
		// Animation
		SDAAnimationRef				mSDAAnimation;
		// SDAWebsocket
		SDAWebsocketRef				mSDAWebsocket;
		// lights4events
		void						colorWrite();
		// MIDI
		vector<midiInput>			mMidiInputs;
		// midi inputs: couldn't make a vector
		midi::Input					mMidiIn0;
		midi::Input					mMidiIn1;
		midi::Input					mMidiIn2;
		midi::Input					mMidiIn3;
		void						midiListener(midi::Message msg);
		// midi output
		midi::MidiOut				mMidiOut0;
		midi::MidiOut				mMidiOut1;
		midi::MidiOut				mMidiOut2;
		midi::MidiOut				mMidiOut3;
		vector<midiOutput>			mMidiOutputs;
		string						midiControlType;
		int							midiControl;
		int							midiPitch;
		int							midiVelocity;
		float						midiNormalizedValue;
		int							midiValue;
		int							midiChannel;
		// midimix solo mode
		bool						midiSticky; 
		bool						midiStickyPrevValue;
		int							midiStickyPrevIndex;

		int							mSelectedWarp;
		int							mSelectedFboA;
		int							mSelectedFboB;

		static const int			MAX = 16;

	};
}

