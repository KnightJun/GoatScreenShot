#include "GoatScreenshot.h"
#include <QApplication>
#include <QScreen>
#include <QWindow>
#include <QDesktopWidget>
#include <QDebug>
#ifdef Q_OS_WIN32
#include "GoatScreenshotGDI.h"
#include "GoatScreenshotDXGI.h"
#elif defined(Q_OS_MACOS)
#include "GoatScreenshotMac.h"
#endif
GoatScreenshot::GoatScreenshot(QRect shotRect, QScreen *screen, bool withCursor)
{
	if(screen == nullptr){
		screen = QApplication::primaryScreen();
	}
	if(shotRect.isEmpty()){
		shotRect = screen->geometry();
	}
	mShotRect = shotRect;
	mWithCursor = withCursor;
	mScreen = screen;
};
#ifdef Q_OS_WIN32
GoatScreenshot* GoatScreenshot::Create(QRect shotRect, QScreen *screen, bool withCursor)
{
	GoatScreenshot* retObj;
	// if(QSysInfo::windowsVersion() == QSysInfo::WV_10_0){
	// 	qInfo() << "init GoatScreenshotDXGI";
	// 	retObj = new GoatScreenshotDXGI(shotRect, screen, withCursor);
	// }else{
		qInfo() << "init GoatScreenshotGDI";
		retObj = new GoatScreenshotGDI(shotRect, screen, withCursor);
	// }
	return retObj;
}
#endif

#ifdef Q_OS_MACOS
GoatScreenshot* GoatScreenshot::Create(QRect shotRect, QScreen *screen, bool withCursor)
{
	GoatScreenshot* retObj;
	qInfo() << "init GoatScreenshotGDI";
	retObj = new GoatScreenshotMac(shotRect, screen, withCursor);
	return retObj;
}
#endif

bool GoatScreenshot::changeShotRect(QRect shotRect)
{
	mShotRect = shotRect;
	return true;
}

QImage GoatScreenshot::screenImageWithMouse()
{
	QImage frame = this->screenImage();
	if(!this->hasMouse())return frame;
	QPainter p(&frame);
	p.drawImage(this->mouseImagePoint(), this->mouseImage());
	return frame;
}

QImage GoatScreenshot::quickShot(QRect shotRect, QScreen *screen, bool withCursor)
{
	GoatScreenshot *shotObj = Create(shotRect, screen, withCursor);
	QImage image;
	shotObj->getNextFrame();
	image = shotObj->screenImage();
	GoatScreenshot::drawImageTo(image, shotObj->mouseImage(), shotObj->mouseImagePoint());
	delete shotObj;
	return image;
}

void GoatScreenshot::drawImageTo(QImage &dst, const QImage &target, const QPoint &pt)
{
	QPainter p(&dst);
	p.drawImage(pt, target);
}

QString GoatScreenshot::screenInfo()
{
    int i = 0;
    QString ret;
    QList<QScreen *> screenList = QApplication::screens();
    for (QScreen *scr : screenList)
    {
        ret += QString("Screen(%1) '%2' [%3, %4 %5x%6]: ")
            .arg(i)
			.arg(scr->name())
            .arg(scr->geometry().x())
            .arg(scr->geometry().y())
            .arg(scr->geometry().width())
            .arg(scr->geometry().height());
        ret += "\n    manufacturer: " + scr->manufacturer();
        ret += "\n    model: " + scr->model();
        ret += "\n    serialNumber: " + scr->serialNumber();
		ret += "\n";
        i++;
    }
	return ret;
}

QScreen *GoatScreenshot::getScreenByMouse()
{
	QPoint globalCursorPos = QCursor::pos();
	int mouseScreen = qApp->desktop()->screenNumber(globalCursorPos);
	return qApp->desktop()->screen(mouseScreen)->windowHandle()->screen();
}

QScreen *GoatScreenshot::getScreenByWidget(QWidget *widget)
{
	return widget->windowHandle()->screen();
}

QScreen *GoatScreenshot::getScreenByNumber(int number)
{
	return qApp->desktop()->screen(number)->windowHandle()->screen();
}