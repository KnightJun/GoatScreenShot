#pragma once
#include "GoatScreenshot.h"
#include <QImage>
#include <QPixmap>
#include <QPainter>

#ifndef BUILD_STATIC
# if defined(GoatScreenShot_LIB)
#  define SCREENSHOTVIEW_EXPORT Q_DECL_EXPORT
# else
#  define SCREENSHOTVIEW_EXPORT Q_DECL_IMPORT
# endif
#else
# define SCREENSHOTVIEW_EXPORT
#endif
struct DXGIParams;
struct _PTR_INFO;
class SCREENSHOTVIEW_EXPORT GoatScreenshotDXGI : public GoatScreenshot
{
public:
	~GoatScreenshotDXGI();
	GoatScreenshotDXGI(QRect shotRect, QScreen *screen, bool withCursor);
	virtual bool getNextFrame();
	QImage screenImage(){return mScreenImage;};
	virtual QImage mouseImage(){return mMouseImage;};
	virtual QPoint mouseImagePoint(){return mMouseImagePoint;};
	bool hasMouse(){return mHasMouse;};

	friend int DrawMouse(GoatScreenshotDXGI* goatShot);
private:
	QImage mScreenImage;
	QImage mMouseImage;
	QPoint mMouseImagePoint;
	bool mHasMouse;
	DXGIParams *DXGI;
    uint mMetaDataSize = 0;
    uchar* mMetaDataBuffer = nullptr;
	_PTR_INFO* mPtrInfo = nullptr; 
	uint mOutputNumber = 0;
};
