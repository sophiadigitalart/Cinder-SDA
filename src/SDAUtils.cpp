#include "SDAUtils.h"

using namespace SophiaDigitalArt;

SDAUtils::SDAUtils(SDASettingsRef aSDASettings)
{
	mSDASettings = aSDASettings;
	//CI_LOG_V("SDAUtils constructor");
	x1LeftOrTop = 0;
	y1LeftOrTop = 0;
	x2LeftOrTop = mSDASettings->mFboWidth;
	y2LeftOrTop = mSDASettings->mFboHeight;
	x1RightOrBottom = 0;
	y1RightOrBottom = 0;
	x2RightOrBottom = mSDASettings->mFboWidth;
	y2RightOrBottom = mSDASettings->mFboHeight;

}
float SDAUtils::formatFloat(float f)
{
	int i;
	f *= 100;
	i = ((int)f) / 100;
	return (float)i;
}

void SDAUtils::setup()
{
	
}

int SDAUtils::getWindowsResolution()
{
	mSDASettings->mDisplayCount = 0;
	int w = Display::getMainDisplay()->getWidth();
	int h = Display::getMainDisplay()->getHeight();

	// Display sizes
	if (mSDASettings->mAutoLayout)
	{
		mSDASettings->mMainWindowWidth = w;
		mSDASettings->mMainWindowHeight = h;
		mSDASettings->mRenderX = mSDASettings->mMainWindowWidth;
		// for MODE_MIX and triplehead(or doublehead), we only want 1/3 of the screen centered	
		for (auto display : Display::getDisplays())
		{
			//CI_LOG_V("SDAUtils Window #" + toString(mSDASettings->mDisplayCount) + ": " + toString(display->getWidth()) + "x" + toString(display->getHeight()));
			mSDASettings->mDisplayCount++;
			mSDASettings->mRenderWidth = display->getWidth();
			mSDASettings->mRenderHeight = display->getHeight();
		}
	}
	else
	{
		for (auto display : Display::getDisplays())
		{
			//CI_LOG_V("SDAUtils Window #" + toString(mSDASettings->mDisplayCount) + ": " + toString(display->getWidth()) + "x" + toString(display->getHeight()));
			mSDASettings->mDisplayCount++;
		}
	}
	mSDASettings->mRenderY = 0;

	//CI_LOG_V("SDAUtils mMainDisplayWidth:" + toString(mSDASettings->mMainWindowWidth) + " mMainDisplayHeight:" + toString(mSDASettings->mMainWindowHeight));
	//CI_LOG_V("SDAUtils mRenderWidth:" + toString(mSDASettings->mRenderWidth) + " mRenderHeight:" + toString(mSDASettings->mRenderHeight));
	//CI_LOG_V("SDAUtils mRenderX:" + toString(mSDASettings->mRenderX) + " mRenderY:" + toString(mSDASettings->mRenderY));
	//mSDASettings->mRenderResoXY = vec2(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight);
	// in case only one screen , render from x = 0
	if (mSDASettings->mDisplayCount == 1) mSDASettings->mRenderX = 0;
	splitWarp(mSDASettings->mFboWidth, mSDASettings->mFboHeight);
	
	return w;
}
void SDAUtils::splitWarp(int fboWidth, int fboHeight) {

	x1LeftOrTop = x1RightOrBottom = 0;
	y1LeftOrTop = y1RightOrBottom = 0;
	x2LeftOrTop = x2RightOrBottom = mSDASettings->mFboWidth;
	y2LeftOrTop = y2RightOrBottom = mSDASettings->mFboHeight;

	if (mSDASettings->mSplitWarpH) {
		x2LeftOrTop = (fboWidth / 2) - 1;

		x1RightOrBottom = fboWidth / 2;
		x2RightOrBottom = fboWidth;
	}
	else if (mSDASettings->mSplitWarpV) {
		y2LeftOrTop = (fboHeight / 2) - 1;
		y1RightOrBottom = fboHeight / 2;
		y2RightOrBottom = fboHeight;
	}
	else
	{
		// no change
	}
	mSrcAreaLeftOrTop = Area(x1LeftOrTop, y1LeftOrTop, x2LeftOrTop, y2LeftOrTop);
	mSrcAreaRightOrBottom = Area(x1RightOrBottom, y1RightOrBottom, x2RightOrBottom, y2RightOrBottom);

}
void SDAUtils::moveX1LeftOrTop(int x1) {
	x1LeftOrTop = x1;
	mSrcAreaLeftOrTop = Area(x1LeftOrTop, y1LeftOrTop, x2LeftOrTop, y2LeftOrTop);
}
void SDAUtils::moveY1LeftOrTop(int y1) {
	y1LeftOrTop = y1;
	mSrcAreaLeftOrTop = Area(x1LeftOrTop, y1LeftOrTop, x2LeftOrTop, y2LeftOrTop);
}

Area SDAUtils::getSrcAreaLeftOrTop() {
	return mSrcAreaLeftOrTop;
}
Area SDAUtils::getSrcAreaRightOrBottom() {
	return mSrcAreaRightOrBottom;
}

fs::path SDAUtils::getPath(string path)
{
	fs::path p = app::getAssetPath("");
	if (path.length() > 0) { p += fs::path("/" + path); }
	return p;
}
string SDAUtils::getFileNameFromFullPath(string path)
{
	fs::path fullPath = path;
	return fullPath.filename().string();
}
