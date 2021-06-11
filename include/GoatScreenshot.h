#pragma once
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
class QScreen;
class SCREENSHOTVIEW_EXPORT GoatScreenshot
{
public:
	virtual ~GoatScreenshot(){};
	static GoatScreenshot* Create(QRect shotRect = QRect(), QScreen *screen = nullptr, bool withCursor = true);
	static QString screenInfo();
	static QScreen *getScreenByMouse();
	static QScreen *getScreenByWidget(QWidget *widget);
	static QScreen *getScreenByNumber(int number);
	static QImage quickShot(QRect shotRect = QRect(), QScreen *screen = nullptr, bool withCursor = true);
	static void drawImageTo(QImage &dst, const QImage &target, const QPoint &pt);
	virtual bool getNextFrame() = 0;
	virtual QImage screenImage() = 0;
	virtual QImage screenImageWithMouse();
	virtual bool hasMouse() = 0;
	virtual QImage mouseImage() = 0;
	virtual QPoint mouseImagePoint() = 0;
	virtual bool changeShotRect(QRect shotRect);

protected:
	GoatScreenshot(QRect shotRect, QScreen *screen, bool withCursor);
	QRect mShotRect;
	bool mWithCursor;
	int screenNum;
	QScreen *mScreen;
};
