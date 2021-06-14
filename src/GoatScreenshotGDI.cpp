#include "GoatScreenshotGDI.h"
#include <windows.h>
#include <QtWinExtras>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QDebug>
#include "ImageHandle.h"

GoatScreenshotGDI::GoatScreenshotGDI(QRect shotRect, QScreen *screen, bool withCursor)
	:GoatScreenshot(shotRect, screen, withCursor)
{
}

GoatScreenshotGDI::~GoatScreenshotGDI(){

}

bool GoatScreenshotGDI::getNextFrame()
{
	mScreenImage = mScreen->grabWindow(0, mShotRect.x(), mShotRect.y(),
		mShotRect.width(), mShotRect.height()).toImage();
	if (mWithCursor) {
		mHasMouse = GoatScreenshotGDI::getMouseImage(mShotRect, mScreenImage, mMouseImage, mMouseImagePoint);
	}
	return true;
}
bool BitmapInfoToBuffer(PBITMAPINFO pbmi, HBITMAP hbm,uchar* buffer)
{
	HDC hDC = CreateCompatibleDC(nullptr);
	bool ret = true;
	HBITMAP hBmpOld = (HBITMAP)SelectObject(hDC, (HGDIOBJ)hbm);
	if (!GetDIBits(hDC, hbm, 0, pbmi->bmiHeader.biHeight, (LPVOID)buffer, pbmi, DIB_RGB_COLORS))
	{
		ret = false;
	}
	SelectObject(hDC, hBmpOld);
	DeleteDC(hDC);
	return ret;
}

void DrawHICON(QImage &image, HICON icon, int x = 0, int y =0)
{
	HBITMAP hBmp = QtWin::imageToHBITMAP(image);
	HDC hdc0 = GetDC(0);
	HDC hdcMem = CreateCompatibleDC(hdc0);
	HBITMAP hBmpOld = (HBITMAP)SelectObject(hdcMem, hBmp);
	DrawIconEx(hdcMem, x, y,
		icon, 0, 0, 0, NULL, DI_NORMAL | DI_COMPAT);
	image = QtWin::imageFromHBITMAP(hBmp);
	DeleteObject(hBmp);
	DeleteObject(hBmpOld);
	ReleaseDC(NULL, hdc0);
	/////////////////////////////////////////
	DeleteDC(hdcMem);
}

QSize GetSize(ICONINFO info) {
	int iconWidth, icomHeight;
	BITMAP bmp = { 0 };
	if (info.hbmColor) {
		const int nWrittenBytes = GetObject(info.hbmColor, sizeof(bmp), &bmp);
		iconWidth = bmp.bmWidth;
		icomHeight = bmp.bmHeight;
		DeleteObject(info.hbmColor);
	}
	else if (info.hbmMask) {
		const int nWrittenBytes = GetObject(info.hbmMask, sizeof(bmp), &bmp);
		iconWidth = bmp.bmWidth;
		icomHeight = bmp.bmHeight / 2;
		DeleteObject(info.hbmMask);
	}
	return QSize(iconWidth, icomHeight);
}

bool GoatScreenshotGDI::getMouseImage(
	const QRect shotRect, const QImage &screenImage, 
	QImage &mouseImage, QPoint &mouseImagePoint)
{
	CURSORINFO ci = {0};
	ICONINFO info = {0};
	BITMAP bmp = { 0 };
	bool result;
	bool bRes;
	ci.cbSize = sizeof(CURSORINFO);
	bRes = GetCursorInfo(&ci);
	bRes = GetIconInfo(ci.hCursor, &info);
	QSize mouseSize = GetSize(info);
	mouseImagePoint = QPoint(ci.ptScreenPos.x - info.xHotspot, ci.ptScreenPos.y - info.yHotspot);
	
	if (!shotRect.intersects(QRect(mouseImagePoint, mouseSize))) {
		return false;
	}
	mouseImagePoint  = mouseImagePoint - shotRect.topLeft();
	QRect mouseRect = QRect(mouseImagePoint, mouseSize);
	mouseImage = screenImage.copy(mouseRect);
	DrawHICON(mouseImage, ci.hCursor);
	// mouseImage.save("mouseImage.png");
	return true;
}
