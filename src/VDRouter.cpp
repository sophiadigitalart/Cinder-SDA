#include "VDRouter.h"

using namespace videodromm;

VDRouter::VDRouter(VDSettingsRef aVDSettings, VDAnimationRef aVDAnimation, VDWebsocketRef aVDWebsocket) {
	mVDSettings = aVDSettings;
	mVDAnimation = aVDAnimation;
	mVDWebsocket = aVDWebsocket;
	CI_LOG_V("VDRouter constructor");
	mFBOAChanged = false;
	mFBOBChanged = false;
	// midi
	if (mVDSettings->mMIDIOpenAllInputPorts) midiSetup();
	// Osc
	if (mVDSettings->mOSCEnabled) {
		mOscReceiver = std::make_shared<osc::ReceiverUdp>(mVDSettings->mOSCReceiverPort);
		mOscReceiver->setListener("/*",
			[&](const osc::Message &msg) {
			// touchosc
			bool found = false;
			string ctrl = "";
			int index = -1;
			int i = 0;
			float f = 1.0f;
			stringstream ss;	
			vec2 vv = vec2(0.0f);
			string addr = msg.getAddress();
			// handle all msg without page integer first
			// midi cc in osc
			ctrl = "/cc";
			index = addr.find(ctrl);
			if (index != std::string::npos)
			{
				found = true;
				i = msg[0].flt();
				f = msg[1].flt() / 128;
				mVDAnimation->setFloatUniformValueByIndex(i, f);
				ss << "midi from OSC " << addr << " " << i << " value " << f;
				mVDSettings->mMidiMsg = ss.str();
			}
			if (!found)
			{
				// accxyz
				ctrl = "accxyz";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
				}
			}
			if (!found)
			{
				// ableton link tempo from ofx abletonLinkToWebsocket
				ctrl = "/live/tempo";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
					mVDAnimation->setAutoBeatAnimation(false);
					mVDAnimation->setBpm(msg[0].flt());
				}
			}
			if (!found)
			{
				// int32 0 or 1 play from Transthor
				ctrl = "/play";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
				}
			}
			if (!found)
			{
				// float ticks from Transthor
				ctrl = "/ticks";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
					f = msg[0].flt();
					mVDAnimation->useTimeWithTempo();
					mVDAnimation->setFloatUniformValueByIndex(mVDSettings->ITIME, f);
					//stringstream ss;
					ss << addr << " " << f;
					//CI_LOG_I("OSC: " << ctrl << " addr: " << addr);
					mVDSettings->mMsg = ss.str();
					//mVDAnimation->setFloatUniformValueByIndex(mVDSettings->IELAPSED, msg[0].flt());
				}
			}
			if (!found)
			{
				/*	float tempo from Transthor
					SOS 160.0
					HumanET	150
					Hoover	135
				*/
				ctrl = "/tempo";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
					mVDAnimation->useTimeWithTempo();
					f = msg[0].flt();
					mVDAnimation->setBpm(f);
					//CI_LOG_I("tempo:" + toString(mVDAnimation->getBpm()));
				}
			}
			if (!found)
			{
				//32 floats from Speckthor
				ctrl = "/Spectrum";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
					mVDAnimation->maxVolume = 0.0f;
					for (int i = 0; i < msg.getNumArgs(); i++) {
						// get the argument type 'f'
						if (msg.getArgType(i) == ArgType::FLOAT) {
							float f = msg[i].flt() * 200.0f;
							if (f > mVDAnimation->maxVolume)
							{
								mVDAnimation->maxVolume = f;
							}
							mVDAnimation->iFreqs[i] = f;
							if (i == mVDAnimation->getFreqIndex(0)) mVDAnimation->setFloatUniformValueByName("iFreq0", f);
							if (i == mVDAnimation->getFreqIndex(1)) mVDAnimation->setFloatUniformValueByName("iFreq1", f);
							if (i == mVDAnimation->getFreqIndex(2)) mVDAnimation->setFloatUniformValueByName("iFreq2", f);
							if (i == mVDAnimation->getFreqIndex(3)) mVDAnimation->setFloatUniformValueByName("iFreq3", f);
						}
					}				
				}
			}
			if (!found)
			{
				// int32 1 to 4 beat from Transthor
				ctrl = "/beat";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;			
					mVDAnimation->setIntUniformValueByIndex(mVDSettings->IBEAT, msg[0].int32() - 1);
					mVDAnimation->setIntUniformValueByIndex(
						mVDSettings->IBARBEAT,
						mVDAnimation->getIntUniformValueByIndex(mVDSettings->IBAR) * 4 + mVDAnimation->getIntUniformValueByIndex(mVDSettings->IBEAT));

					//CI_LOG_I("beat:" + toString(mVDSettings->IBEAT) + " " + toString(mVDAnimation->getIntUniformValueByIndex(mVDSettings->IBEAT)));
	
				}
			}
			if (!found)
			{
				// int32 bar from Transthor
				ctrl = "/bar";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
					// TODO test if useless:
					int previousBar = mVDAnimation->getIntUniformValueByIndex(mVDSettings->IBAR);

					if (previousBar != msg[0].int32()) {
						mVDSettings->iBarDuration = mVDAnimation->getFloatUniformValueByIndex(mVDSettings->ITIME) - mBarStart;
						mBarStart = mVDAnimation->getFloatUniformValueByIndex(mVDSettings->ITIME);
					}
					// TODO END
					mVDAnimation->setIntUniformValueByIndex(mVDSettings->IBAR, msg[0].int32());
					mVDAnimation->setIntUniformValueByIndex(
						mVDSettings->IBARBEAT,
						mVDAnimation->getIntUniformValueByIndex(mVDSettings->IBAR) * 4 + mVDAnimation->getIntUniformValueByIndex(mVDSettings->IBEAT));

				}
			}

			if (!found)
			{
				ctrl = "link";
				index = addr.find(ctrl);
				if (index != std::string::npos)
				{
					found = true;
					double d0 = msg[0].dbl(); // tempo
					mVDAnimation->setBpm(d0);
					double d1 = msg[1].dbl();
					int d2 = msg[2].int32();
					mVDWebsocket->changeIntValue(mVDSettings->IBEAT, d2);
				}
			}
			if (!found)
			{
				// touchosc
				int page = 0;
				try {
					// CHECK exception if not integer
					page = std::stoi(addr.substr(1, 1)); // 1 to 4

					int lastSlashIndex = addr.find_last_of("/"); // 0 2
					// if not a page selection
					if (addr.length() > 2) {
						ctrl = "multifader1";
						index = addr.find(ctrl);
						if (index != std::string::npos)
						{
							found = true;
							f = msg[0].flt();
							i = std::stoi(addr.substr(lastSlashIndex + 1)) + 8;
							mVDAnimation->setFloatUniformValueByIndex(i, f);
						}

						if (!found)
						{
							ctrl = "multifader2";
							index = addr.find(ctrl);
							if (index != std::string::npos)
							{
								found = true;
								f = msg[0].flt();
								i = std::stoi(addr.substr(lastSlashIndex + 1)) + 32; // 24 + 8
								mVDAnimation->setFloatUniformValueByIndex(i, f);
							}
						}
						
						if (!found)
						{
							ctrl = "multifader";
							index = addr.find(ctrl);
							if (index != std::string::npos)
							{
								found = true;
								f = msg[0].flt();
								i = std::stoi(addr.substr(lastSlashIndex + 1)) + 56; // 48 + 8
								mVDAnimation->setFloatUniformValueByIndex(i, f);
							}
						}
						if (!found)
						{
							ctrl = "fader";
							index = addr.find(ctrl);
							if (index != std::string::npos)
							{
								found = true;
								f = msg[0].flt();
								i = std::stoi(addr.substr(index + ctrl.length()));
								mVDAnimation->setFloatUniformValueByIndex(i, f);// starts at 1: mVDSettings->IFR G B
							}
						}
						if (!found)
						{
							ctrl = "rotary";
							index = addr.find(ctrl);
							if (index != std::string::npos)
							{
								found = true;
								f = msg[0].flt();
								i = std::stoi(addr.substr(index + ctrl.length())) + 10;
								mVDAnimation->setFloatUniformValueByIndex(i, f);
							}
						}
						if (!found)
						{
							// toggle
							ctrl = "multitoggle";
							index = addr.find(ctrl);
							if (!found && index != std::string::npos)
							{
								found = true;
								// /2/multitoggle/5/3
							}
						}
						if (!found)
						{
							// toggle
							ctrl = "toggle";
							index = addr.find(ctrl);
							if (!found && index != std::string::npos)
							{
								found = true;
								f = msg[0].flt();
								i = std::stoi(addr.substr(index + ctrl.length())) + 80;
								mVDAnimation->toggleValue(i);
							}
						}
						if (!found)
						{
							// push
							ctrl = "push";
							index = addr.find(ctrl);
							if (!found && index != std::string::npos)
							{
								found = true;
								f = msg[0].flt();
								i = std::stoi(addr.substr(index + ctrl.length())) + 80;
								mVDAnimation->toggleValue(i);
							}
						}
						if (!found)
						{
							// xy
							ctrl = "xy1";
							index = addr.find(ctrl);
							if (!found && index != std::string::npos)
							{
								found = true;
								vv = vec2(msg[0].flt(), msg[1].flt());
							}
						}
						if (!found)
						{
							// xy
							ctrl = "xy2";
							index = addr.find(ctrl);
							if (!found && index != std::string::npos)
							{
								found = true;
								vv = vec2(msg[0].flt(), msg[1].flt());
							}
						}
						if (!found)
						{
							// xy
							ctrl = "xy";
							index = addr.find(ctrl);
							if (!found && index != std::string::npos)
							{
								found = true;
								vv = vec2(msg[0].flt(), msg[1].flt());
							}
						}
					}
				}
				catch (const std::exception& e) {
					ss << addr << " not integer";
					mVDSettings->mOSCMsg = ss.str();
					CI_LOG_E("not integer: " << addr);
				}
			}
			if (found) {
				if (addr != "/play") mVDSettings->mOSCMsg = addr;
				// nope too much disk io CI_LOG_I("OSC: " << ctrl << " addr: " << addr);
			}
			else {
				CI_LOG_E("not handled: " << msg.getNumArgs() << " addr: " << addr);
				mVDSettings->mOSCMsg = "not handled: " + addr;
				mVDSettings->mMsg = "osc not handled: " + addr;
			}
		});

		try {
			// Bind the receiver to the endpoint. This function may throw.
			mOscReceiver->bind();
		}
		catch (const osc::Exception &ex) {
			CI_LOG_E("Error binding: " << ex.what() << " val: " << ex.value());
		}

		// UDP opens the socket and "listens" accepting any message from any endpoint. The listen
		// function takes an error handler for the underlying socket. Any errors that would
		// call this function are because of problems with the socket or with the remote message.
		mOscReceiver->listen(
			[](asio::error_code error, protocol::endpoint endpoint) -> bool {
			if (error) {
				CI_LOG_E("Error Listening: " << error.message() << " val: " << error.value() << " endpoint: " << endpoint);
				return false;
			}
			else
				return true;
		});
	}
	
	mSelectedWarp = 0;
	mSelectedFboA = 1;
	mSelectedFboB = 2;
}

void VDRouter::shutdown() {

	mMidiIn0.closePort();
	mMidiIn1.closePort();
	mMidiIn2.closePort();
	mMidiOut0.closePort();
	mMidiOut1.closePort();
	mMidiOut2.closePort();

}

void VDRouter::midiSetup() {
	stringstream ss;
	ss << "setupMidi ";
	CI_LOG_V("midiSetup: " + ss.str());
	if (mMidiIn0.getNumPorts() > 0)
	{
		mMidiIn0.listPorts();
		for (int i = 0; i < mMidiIn0.getNumPorts(); i++)
		{
			bool alreadyListed = false;
			for (int j = 0; j < mMidiInputs.size(); j++)
			{
				if (mMidiInputs[j].portName == mMidiIn0.getPortName(i)) alreadyListed = true;
			}
			if (!alreadyListed) {
				midiInput mIn;
				mIn.portName = mMidiIn0.getPortName(i);
				mMidiInputs.push_back(mIn);
				if (mVDSettings->mMIDIOpenAllInputPorts) {
					openMidiInPort(i);
					mMidiInputs[i].isConnected = true;
					ss << "Opening MIDI in port " << i << " " << mMidiInputs[i].portName;
				}
				else {
					mMidiInputs[i].isConnected = false;
					ss << "Available MIDI in port " << i << " " << mMidiIn0.getPortName(i);
				}
			}
		}
	}
	else {
		ss << "no midi in ports found";
	}

	// midi out
	//mMidiOut0.getPortList();
	if (mMidiOut0.getNumPorts() > 0) {
		for (int i = 0; i < mMidiOut0.getNumPorts(); i++)
		{
			bool alreadyListed = false;
			for (int j = 0; j < mMidiOutputs.size(); j++)
			{
				if (mMidiOutputs[j].portName == mMidiOut0.getPortName(i)) alreadyListed = true;
			}
			if (!alreadyListed) {
				midiOutput mOut;
				mOut.portName = mMidiOut0.getPortName(i);
				mMidiOutputs.push_back(mOut);

				mMidiOutputs[i].isConnected = false;
				ss << "Available MIDI output port " << i << " " << mMidiOut0.getPortName(i);

			}
		}
	}
	else {
		ss << "no midi out Ports found";
	}
	midiControlType = "none";
	midiControl = midiPitch = midiVelocity = midiNormalizedValue = midiValue = midiChannel = 0;
	ss << std::endl;
	mVDSettings->mNewMsg = true;
	mVDSettings->mMidiMsg = ss.str();
	CI_LOG_V(ss.str());
}

void VDRouter::openMidiInPort(int i) {
	CI_LOG_V("openMidiInPort: " + toString(i));
		stringstream ss;
		if (i < mMidiIn0.getNumPorts()) {
			if (i == 0) {
				mMidiIn0.openPort(i);
				mMidiIn0.midiSignal.connect(std::bind(&VDRouter::midiListener, this, std::placeholders::_1));
			}
			if (i == 1) {
				mMidiIn1.openPort(i);
				mMidiIn1.midiSignal.connect(std::bind(&VDRouter::midiListener, this, std::placeholders::_1));
			}
			if (i == 2) {
				mMidiIn2.openPort(i);
				mMidiIn2.midiSignal.connect(std::bind(&VDRouter::midiListener, this, std::placeholders::_1));
			}
		}
		mMidiInputs[i].isConnected = true;
		ss << "Opening MIDI in port " << i << " " << mMidiInputs[i].portName << std::endl;
		mVDSettings->mMsg = ss.str();
		CI_LOG_V(ss.str());
		mVDSettings->mNewMsg = true;
}
void VDRouter::closeMidiInPort(int i) {

	if (i == 0)
	{
		mMidiIn0.closePort();
	}
	if (i == 1)
	{
		mMidiIn1.closePort();
	}
	if (i == 2)
	{
		mMidiIn2.closePort();
	}
	mMidiInputs[i].isConnected = false;

}
void VDRouter::midiOutSendNoteOn(int i, int channel, int pitch, int velocity) {

	if (i == 0)
	{
		if (mMidiOutputs[i].isConnected) mMidiOut0.sendNoteOn(channel, pitch, velocity);
	}
	if (i == 1)
	{
		if (mMidiOutputs[i].isConnected) mMidiOut1.sendNoteOn(channel, pitch, velocity);
	}
	if (i == 2)
	{
		if (mMidiOutputs[i].isConnected) mMidiOut2.sendNoteOn(channel, pitch, velocity);
	}

}
void VDRouter::openMidiOutPort(int i) {

	stringstream ss;
	ss << "Port " << i;
	if (i < mMidiOutputs.size()) {
		if (i == 0) {
			if (mMidiOut0.openPort(i)) {
				mMidiOutputs[i].isConnected = true;
				ss << " Opened MIDI out port " << i << " " << mMidiOutputs[i].portName;
				mMidiOut0.sendNoteOn(1, 40, 64);
			}
			else {
				ss << " Can't open MIDI out port " << i << " " << mMidiOutputs[i].portName;
			}
		}
		if (i == 1) {
			if (mMidiOut1.openPort(i)) {
				mMidiOutputs[i].isConnected = true;
				ss << " Opened MIDI out port " << i << " " << mMidiOutputs[i].portName;
				mMidiOut1.sendNoteOn(1, 40, 64);
			}
			else {
				ss << " Can't open MIDI out port " << i << " " << mMidiOutputs[i].portName;
			}
		}
		if (i == 2) {
			if (mMidiOut2.openPort(i)) {
				mMidiOutputs[i].isConnected = true;
				ss << " Opened MIDI out port " << i << " " << mMidiOutputs[i].portName;
				mMidiOut2.sendNoteOn(1, 40, 64);
			}
			else {
				ss << " Can't open MIDI out port " << i << " " << mMidiOutputs[i].portName;
			}
		}
	}
	ss << std::endl;
	mVDSettings->mMsg = ss.str();
	mVDSettings->mNewMsg = true;
	CI_LOG_V(ss.str());
}
void VDRouter::closeMidiOutPort(int i) {

	if (i == 0)
	{
		mMidiOut0.closePort();
	}
	if (i == 1)
	{
		mMidiOut1.closePort();
	}
	if (i == 2)
	{
		mMidiOut2.closePort();
	}
	mMidiOutputs[i].isConnected = false;

}

void VDRouter::midiListener(midi::Message msg) {
	stringstream ss;
	midiChannel = msg.channel;
	switch (msg.status)
	{
	case MIDI_CONTROL_CHANGE:
		midiControlType = "/cc";
		midiControl = msg.control;
		midiValue = msg.value;
		midiNormalizedValue = lmap<float>(midiValue, 0.0, 127.0, 0.0, 1.0);
		ss << "MIDI cc Chn: " << midiChannel << " CC: " << midiControl  << " Val: " << midiValue << " NVal: " << midiNormalizedValue;
		CI_LOG_V("Midi: " + ss.str());

		if (midiControl > 20 && midiControl < 49) {
			/*if (midiControl > 20 && midiControl < 29) {
				mSelectedWarp = midiControl - 21;
			} 
			if (midiControl > 40 && midiControl < 49) {
				mSelectedFboB = midiControl - 41;
				mVDAnimation->setIntUniformValueByIndex(mVDSettings->IFBOB, mSelectedFboB);
			}
			*/
			//if (midiControl > 30 && midiControl < 39) {
				mVDWebsocket->changeFloatValue(midiControl, midiNormalizedValue);
				//mSelectedFboA = midiControl - 31;
				//mVDAnimation->setIntUniformValueByIndex(mVDSettings->IFBOA, mSelectedFboA);
			//}
			
		}
		else {
			updateParams(midiControl, midiNormalizedValue);
		}
		//mWebSockets->write("{\"params\" :[{" + controlType);
		break;
	case MIDI_NOTE_ON:
		/*
		TODO nano notes instad of cc
		if (midiControl > 20 && midiControl < 29) {
				mSelectedWarp = midiControl - 21;
			}
			if (midiControl > 30 && midiControl < 39) {
				mSelectedFboA = midiControl - 31;
				mVDAnimation->setIntUniformValueByIndex(mVDSettings->IFBOA, mSelectedFboA);
			}
			if (midiControl > 40 && midiControl < 49) {
				mSelectedFboB = midiControl - 41;
				mVDAnimation->setIntUniformValueByIndex(mVDSettings->IFBOB, mSelectedFboB);
			}
		*/
		//midiControlType = "/on";
		//midiPitch = msg.pitch;
		//midiVelocity = msg.velocity;
		//midiNormalizedValue = lmap<float>(midiVelocity, 0.0, 127.0, 0.0, 1.0);
		//// quick hack!
		//mVDAnimation->setFloatUniformValueByIndex(14, 1.0f + midiNormalizedValue);
		midiPitch = msg.pitch;
		// midimix solo mode
		/*if (midiPitch == 27) midiSticky = true;
		if (midiSticky) {
			midiStickyPrevIndex = midiPitch;
			midiStickyPrevValue = mVDAnimation->getBoolUniformValueByIndex(midiPitch + 80);
		}*/
		//mVDAnimation->setBoolUniformValueByIndex(midiPitch + 80, true);

		// This does mVDSession->setFboFragmentShaderIndex(0, midiPitch);
		if (midiPitch < 9) {
			mSelectedFboA = midiPitch;
			mFBOAChanged = true;
		}
		if (midiPitch > 8 && midiPitch < 17) {
			mSelectedFboB = midiPitch - 8;
			mFBOBChanged = true;
		}
		if (midiPitch > 17 && midiPitch < 24) {
			mVDAnimation->setBoolUniformValueByIndex(midiPitch + 80-17, true);
		}
		ss << "MIDI noteon Chn: " << midiChannel << " Pitch: " << midiPitch;
		CI_LOG_V("Midi: " + ss.str());
		break;
	case MIDI_NOTE_OFF:
		midiPitch = msg.pitch;
		if (midiPitch > 17 && midiPitch < 24) {
			mVDAnimation->setBoolUniformValueByIndex(midiPitch + 80 - 17, false);
		}
		// midimix solo mode
		/*if (midiPitch == 27) {
			midiStickyPrevIndex = 0;
			midiSticky = false;
		}
		if (!midiSticky) {*/
			//mVDAnimation->setBoolUniformValueByIndex(midiPitch + 80, false);
		/*}
		else {
			if (midiPitch == midiStickyPrevIndex) {
				mVDAnimation->setBoolUniformValueByIndex(midiPitch + 80, !midiStickyPrevValue);
			}
		}*/
		ss << "MIDI noteoff Chn: " << midiChannel << " Pitch: " << midiPitch;
		CI_LOG_V("Midi: " + ss.str());
		/*midiControlType = "/off";
		midiPitch = msg.pitch;
		midiVelocity = msg.velocity;
		midiNormalizedValue = lmap<float>(midiVelocity, 0.0, 127.0, 0.0, 1.0);*/
		break;
	default:
		break;
	}
	//ss << "MIDI Chn: " << midiChannel << " type: " << midiControlType << " CC: " << midiControl << " Pitch: " << midiPitch << " Vel: " << midiVelocity << " Val: " << midiValue << " NVal: " << midiNormalizedValue << std::endl;
	//CI_LOG_V("Midi: " + ss.str());
	ss << std::endl;
	mVDSettings->mMidiMsg = ss.str();
}

void VDRouter::updateParams(int iarg0, float farg1) {
	if (farg1 > 0.1) {
		//avoid to send twice
		if (iarg0 == 58) {
			// track left		
			mVDSettings->iTrack--;
			if (mVDSettings->iTrack < 0) mVDSettings->iTrack = 0;
		}
		if (iarg0 == 59) {
			// track right
			mVDSettings->iTrack++;
			if (mVDSettings->iTrack > 7) mVDSettings->iTrack = 7;
		}
		if (iarg0 == 60) {
			// set (reset blendmode)
			mVDSettings->iBlendmode = 0;
		}
		if (iarg0 == 61) {
			// right arrow
			mVDSettings->iBlendmode--;
			if (mVDSettings->iBlendmode < 0) mVDSettings->iBlendmode = mVDAnimation->getBlendModesCount() - 1;
		}
		if (iarg0 == 62) {
			// left arrow
			mVDSettings->iBlendmode++;
			if (mVDSettings->iBlendmode > mVDAnimation->getBlendModesCount() - 1) mVDSettings->iBlendmode = 0;
		}
	}
	if (iarg0 > 0 && iarg0 < 9) {
		// sliders 
		mVDWebsocket->changeFloatValue(iarg0, farg1);
	}
	if (iarg0 > 10 && iarg0 < 19) {
		// rotary 
		mVDWebsocket->changeFloatValue(iarg0, farg1);
		// audio multfactor
		if (iarg0 == 13) mVDWebsocket->changeFloatValue(iarg0, (farg1 + 0.01) * 10);
		// exposure
		if (iarg0 == 14) mVDWebsocket->changeFloatValue(iarg0, (farg1 + 0.01) * mVDAnimation->getMaxUniformValueByIndex(14));
		// xfade
		if (iarg0 == mVDSettings->IXFADE) {//18
			mVDWebsocket->changeFloatValue(iarg0, farg1);
			//mVDSettings->xFade = farg1;
			//mVDSettings->xFadeChanged = true;
		}
	}
	// buttons
	if (iarg0 > 20 && iarg0 < 29) {
		// top row
		mVDWebsocket->changeFloatValue(iarg0, farg1);
	}
	if (iarg0 > 30 && iarg0 < 39)
	{
		// middle row
		mVDWebsocket->changeFloatValue(iarg0, farg1);
		//mVDAnimation->setIntUniformValueByIndex(mVDSettings->IFBOA, iarg0 - 31);
	}
	if (iarg0 > 40 && iarg0 < 49) {
		// low row 
		mVDWebsocket->changeFloatValue(iarg0, farg1);
		//mVDAnimation->setIntUniformValueByIndex(mVDSettings->IFBOB, iarg0 - 41);
	}
	//if (iarg0 > 0 && iarg0 < 49) {
		// float values 
		//mVDWebsocket->wsWrite("{\"params\" :[{ \"name\":" + toString(iarg0) + ",\"value\":" + toString(mVDAnimation->getFloatUniformValueByIndex(iarg0)) + "}]}");
	//}
}


void VDRouter::colorWrite()
{
#if defined( CINDER_MSW )
	// lights4events
	char col[97];
	int r = (int)(mVDAnimation->getFloatUniformValueByIndex(1) * 255);
	int g = (int)(mVDAnimation->getFloatUniformValueByIndex(2) * 255);
	int b = (int)(mVDAnimation->getFloatUniformValueByIndex(3) * 255);
	int a = (int)(mVDAnimation->getFloatUniformValueByIndex(4) * 255);
	//sprintf(col, "#%02X%02X%02X", r, g, b);
	sprintf(col, "{\"type\":\"action\", \"parameters\":{\"name\":\"FC\",\"parameters\":{\"color\":\"#%02X%02X%02X%02X\",\"fading\":\"NONE\"}}}", a, r, g, b);
	mVDWebsocket->wsWrite(col);
#endif
}
