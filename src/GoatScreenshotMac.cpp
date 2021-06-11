#include "GoatScreenshotMac.h"
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QDebug>
#include "ImageHandle.h"

GoatScreenshotMac::GoatScreenshotMac(QRect shotRect, QScreen *screen, bool withCursor)
	:GoatScreenshot(shotRect, screen, withCursor)
{
    mHasMouse = false;
}

GoatScreenshotMac::~GoatScreenshotMac(){

}

bool GoatScreenshotMac::getNextFrame()
{
	mScreenImage = mScreen->grabWindow(0, mShotRect.x(), mShotRect.y(),
		mShotRect.width(), mShotRect.height()).toImage();
	if (mWithCursor) {
		mHasMouse = GoatScreenshotMac::getMouseImage(mShotRect, mScreenImage, mMouseImage, mMouseImagePoint);
	}
	return true;
}

bool GoatScreenshotMac::getMouseImage(
	const QRect shotRect, const QImage &screenImage, 
	QImage &mouseImage, QPoint &mouseImagePoint)
{
	return false;
}
