#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

// Settings
#include "VDSettings.h"
// Session
#include "VDSession.h"
// Log
#include "VDLog.h"
// Spout
#include "CiSpoutOut.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace videodromm;

class _TBOX_PREFIX_App : public App {

public:
	_TBOX_PREFIX_App();
	void mouseMove(MouseEvent event) override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;
	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override;
	void fileDrop(FileDropEvent event) override;
	void update() override;
	void draw() override;
	void cleanup() override;
	void toggleUIVisibility() { mVDSession->toggleUI(); };
	void toggleCursorVisibility(bool visible);
private:
	// Settings
	VDSettingsRef					mVDSettings;
	// Session
	VDSessionRef					mVDSession;
	// Log
	VDLogRef						mVDLog;

	string							mError;
	bool							mIsShutDown;
	Anim<float>						mRenderWindowTimer;
	void							positionRenderWindow();
	bool							mFadeInDelay;
	SpoutOut 						mSpoutOut;
};


_TBOX_PREFIX_App::_TBOX_PREFIX_App()
	: mSpoutOut("VD", app::getWindowSize())
{
	// Settings
	mVDSettings = VDSettings::create("_TBOX_PREFIX_App");
	// Session
	mVDSession = VDSession::create(mVDSettings);
	//mVDSettings->mCursorVisible = true;
	toggleCursorVisibility(mVDSettings->mCursorVisible);
	mVDSession->getWindowsResolution();

	mFadeInDelay = true;
	// windows
	mIsShutDown = false;
	mRenderWindowTimer = 0.0f;
	//timeline().apply(&mRenderWindowTimer, 1.0f, 2.0f).finishFn([&] { positionRenderWindow(); });

}
void _TBOX_PREFIX_App::positionRenderWindow() {
	mVDSession->getWindowsResolution();
	mVDSettings->mRenderPosXY = ivec2(mVDSettings->mRenderX, mVDSettings->mRenderY);
	setWindowPos(mVDSettings->mRenderX, mVDSettings->mRenderY);
	setWindowSize(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);
}
void _TBOX_PREFIX_App::toggleCursorVisibility(bool visible)
{
	if (visible)
	{
		showCursor();
	}
	else
	{
		hideCursor();
	}
}
void _TBOX_PREFIX_App::fileDrop(FileDropEvent event)
{
	mVDSession->fileDrop(event);
}
void _TBOX_PREFIX_App::update()
{
	mVDSession->setFloatUniformValueByIndex(mVDSettings->IFPS, getAverageFps());
	mVDSession->update();
}
void _TBOX_PREFIX_App::cleanup()
{
	if (!mIsShutDown)
	{
		mIsShutDown = true;
		CI_LOG_V("shutdown");
		// save settings
		mVDSettings->save();
		mVDSession->save();
		quit();
	}
}
void _TBOX_PREFIX_App::mouseMove(MouseEvent event)
{
	if (!mVDSession->handleMouseMove(event)) {
		// let your application perform its mouseMove handling here
	}
}
void _TBOX_PREFIX_App::mouseDown(MouseEvent event)
{
	if (!mVDSession->handleMouseDown(event)) {
		// let your application perform its mouseDown handling here
		if (event.isRightDown()) { 
		}
	}
}
void _TBOX_PREFIX_App::mouseDrag(MouseEvent event)
{
	if (!mVDSession->handleMouseDrag(event)) {
		// let your application perform its mouseDrag handling here
	}	
}
void _TBOX_PREFIX_App::mouseUp(MouseEvent event)
{
	if (!mVDSession->handleMouseUp(event)) {
		// let your application perform its mouseUp handling here
	}
}

void _TBOX_PREFIX_App::keyDown(KeyEvent event)
{
	if (event.getCode() == KeyEvent::KEY_F12) {
		quit();
	}
	else {
		if (!mVDSession->handleKeyDown(event)) {
			switch (event.getCode()) {
			case KeyEvent::KEY_c:
				// mouse cursor and ui visibility
				mVDSettings->mCursorVisible = !mVDSettings->mCursorVisible;
				toggleCursorVisibility(mVDSettings->mCursorVisible);
				break;
			case KeyEvent::KEY_F11:
				// windows position
				positionRenderWindow();
				break;
			}
		}
	}
}
void _TBOX_PREFIX_App::keyUp(KeyEvent event)
{
	if (!mVDSession->handleKeyUp(event)) {
	}
}

void _TBOX_PREFIX_App::draw()
{
	gl::clear(Color::black());
	if (mFadeInDelay) {
		mVDSettings->iAlpha = 0.0f;
		if (getElapsedFrames() > mVDSession->getFadeInDelay()) {
			mFadeInDelay = false;
			timeline().apply(&mVDSettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}

	//gl::setMatricesWindow(toPixels(getWindowSize()),false);
	gl::setMatricesWindow(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, false);
	gl::draw(mVDSession->getMixTexture(), getWindowBounds());

	// Spout Send
	mSpoutOut.sendViewport();
	getWindow()->setTitle(mVDSettings->sFps + " fps VD");
}

void prepareSettings(App::Settings *settings)
{
	settings->setWindowSize(640, 480);
}

CINDER_APP(_TBOX_PREFIX_App, RendererGl, prepareSettings)
