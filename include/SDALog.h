#pragma once

#include "cinder/Cinder.h"
#include "cinder/app/App.h"
#include "cinder/DataSource.h"
#include "cinder/Utilities.h"
// log
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace SophiaDigitalArt
{
	// stores the pointer to the SDALog instance
	typedef std::shared_ptr<class SDALog> SDALogRef;

	class SDALog {
	public:		
		SDALog();

		static SDALogRef	create()
		{
			return shared_ptr<SDALog>(new SDALog());
		}

	private:

	};


}