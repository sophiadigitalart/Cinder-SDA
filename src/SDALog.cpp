#include "SDALog.h"

using namespace SophiaDigitalArt;

SDALog::SDALog()
{
	auto sysLogger = log::makeLogger<log::LoggerSystem>();
	sysLogger->setLoggingLevel(log::LEVEL_WARNING);
#ifdef _DEBUG
	log::makeLogger<log::LoggerFileRotating>("/tmp/sda", "sda.%Y.%m.%d.txt", false);
#else
	log::makeLogger<log::LoggerFileRotating>("/tmp/sda", "sda.%Y.%m.%d.txt", false);
#endif  // _DEBUG
}
