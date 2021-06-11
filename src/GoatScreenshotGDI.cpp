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
PBITMAPINFO CreateBitmapInfoStruct(HBITMAP hBmp)
{
	BITMAP bmp;
	PBITMAPINFO pbmi;
	WORD    cClrBits;
	// Retrieve the bitmap color format, width, and height. 
	if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp))
		return NULL;

	// Convert the color format to a count of bits. 
	cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
	if (cClrBits == 1)
		cClrBits = 1;
	else if (cClrBits <= 4)
		cClrBits = 4;
	else if (cClrBits <= 8)
		cClrBits = 8;
	else if (cClrBits <= 16)
		cClrBits = 16;
	else if (cClrBits <= 24)
		cClrBits = 24;
	else cClrBits = 32;

	// Allocate memory for the BITMAPINFO structure. (This structure 
	// contains a BITMAPINFOHEADER structure and an array of RGBQUAD 
	// data structures.) 

	if (cClrBits != 24)
		pbmi = (PBITMAPINFO)LocalAlloc(LPTR,
			sizeof(BITMAPINFOHEADER) +
			sizeof(RGBQUAD) * (1 << cClrBits));

	// There is no RGBQUAD array for the 24-bit-per-pixel format. 

	else
		pbmi = (PBITMAPINFO)LocalAlloc(LPTR,
			sizeof(BITMAPINFOHEADER));

	// Initialize the fields in the BITMAPINFO structure. 

	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biWidth = bmp.bmWidth;
	pbmi->bmiHeader.biHeight = bmp.bmHeight;
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
	if (cClrBits < 24)
		pbmi->bmiHeader.biClrUsed = (1 << cClrBits);

	// If the bitmap is not compressed, set the BI_RGB flag. 
	pbmi->bmiHeader.biCompression = BI_RGB;

	// Compute the number of bytes in the array of color 
	// indices and store the result in biSizeImage. 
	// For Windows NT, the width must be DWORD aligned unless 
	// the bitmap is RLE compressed. This example shows this. 
	// For Windows 95/98/Me, the width must be WORD aligned unless the 
	// bitmap is RLE compressed.
	pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits + 31) & ~31) / 8
		* pbmi->bmiHeader.biHeight;
	// Set biClrImportant to 0, indicating that all of the 
	// device colors are important. 
	pbmi->bmiHeader.biClrImportant = 0;
	return pbmi;
}

bool GoatScreenshotGDI::getMouseImage(
	const QRect shotRect, const QImage &screenImage, 
	QImage &mouseImage, QPoint &mouseImagePoint)
{
	CURSORINFO ci = {0};
	ICONINFO info = {0};
	BITMAP bmp = { 0 };
	bool bRes;
	ci.cbSize = sizeof(CURSORINFO);
	bRes = GetCursorInfo(&ci);
	if (!bRes){
		qDebug() << "GetCursorInfo faild";
		return false;
	}
	bRes = GetIconInfo(ci.hCursor, &info);
	if (!bRes){
		qDebug() << "GetIconInfo faild";
		return false;
	}
	mouseImagePoint = QPoint(ci.ptScreenPos.x - info.xHotspot, ci.ptScreenPos.y - info.yHotspot);
	mouseImagePoint  = mouseImagePoint - shotRect.topLeft();
	
	HBITMAP hbm = info.hbmColor ? info.hbmColor : info.hbmMask;
	PBITMAPINFO pBitmapInfo = CreateBitmapInfoStruct(hbm);
	QRect mouseRect = QRect(mouseImagePoint.x(), mouseImagePoint.y(), 
	pBitmapInfo->bmiHeader.biWidth, pBitmapInfo->bmiHeader.biHeight);
	if(hbm == info.hbmMask){
		mouseRect.setHeight(mouseRect.height() / 2);
	}
	if(!mouseRect.intersects(QRect(0, 0, shotRect.width(), shotRect.height()))){
		LocalFree(pBitmapInfo);
		DeleteObject(hbm);
		return false;
	}

	ImageRealloc(mouseImage, mouseRect.width(), mouseRect.height(),
		QImage::Format_ARGB32);
	
	uchar* buffer = new uchar[pBitmapInfo->bmiHeader.biSizeImage];
	pBitmapInfo->bmiHeader.biHeight = -pBitmapInfo->bmiHeader.biHeight;
	BitmapInfoToBuffer(pBitmapInfo, hbm, buffer);
	if(info.hbmColor){
		ImageBuffCopy(mouseImage, buffer);
	} else {
		mouseImage = screenImage.copy(QRect(mouseImagePoint, mouseImage.size()));
		DrawCursorMask(mouseImage, buffer, pBitmapInfo->bmiHeader.biBitCount * pBitmapInfo->bmiHeader.biWidth / 8);
	}
	LocalFree(pBitmapInfo);
	DeleteObject(hbm);
	return true;
}
