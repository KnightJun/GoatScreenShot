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

class SCREENSHOTVIEW_EXPORT GoatScreenshotGDI : public GoatScreenshot
{
public:
	~GoatScreenshotGDI();
	GoatScreenshotGDI(QRect shotRect, QScreen *screen, bool withCursor);
	virtual bool getNextFrame();
	QImage screenImage(){return mScreenImage;};
	virtual QImage mouseImage(){return mMouseImage;};
	static bool getMouseImage(
		const QRect shotRect, const QImage &screenImage, 
		QImage &mouseImage, QPoint &mouseImagePoint);
	virtual QPoint mouseImagePoint(){return mMouseImagePoint;};
	bool hasMouse(){return mHasMouse;};
private:
	QImage mScreenImage;
	QImage mMouseImage;
	QPoint mMouseImagePoint;
	bool mHasMouse;
};
