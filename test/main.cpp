#include <stdio.h>
#include <GoatScreenshot.h>
#include <QtWidgets/QApplication>
#include <gtest/gtest.h>
#include <QDebug>
#include <GoatScreenshotGDI.h>
#include <GoatScreenshotDXGI.h>

TEST(ScreenInfo, info)
{
    qDebug().noquote() << GoatScreenshot::screenInfo();
}

/*
 * 正常测试
 * */
TEST(NormalEncode, GoatScreenshotGDI) {
    GoatScreenshot* gscreen = GoatScreenshot::Create();
    EXPECT_TRUE(gscreen->screenImage().isNull());
    gscreen->getNextFrame();
    EXPECT_FALSE(gscreen->screenImage().isNull());
    qDebug() << "shot size: " << gscreen->screenImage().size();
    qDebug() << "has mouse: " << gscreen->hasMouse();
    qDebug() << "cursor size: " << gscreen->mouseImage().size();
    gscreen->mouseImage().save("mouse.png");
    gscreen->screenImage().save("screenImage.png");
    gscreen->screenImageWithMouse().save("screenImageWithMouse.png");
    delete gscreen;
}

TEST(NormalEncode, QuickShot) {
    QImage img = GoatScreenshot::quickShot();
    EXPECT_FALSE(img.isNull());
    qDebug() << "shot size: " << img.size();
}

// TEST(NormalEncode, NoCursor) {
//     GoatScreenshotGDI gscreen;
//     EXPECT_TRUE(gscreen->screenPixmap()->isNull());
//     gscreen->shotScreen(QRect(), false);
//     EXPECT_FALSE(gscreen->screenPixmap()->isNull());
//     qDebug() << "shot size: " << gscreen->screenPixmap()->size();
//     auto noCursor = gscreen->screenPixmap()->toImage();
//     EXPECT_FALSE(gscreen->showCursor(true));
//     EXPECT_EQ(gscreen->screenPixmap()->size(), noCursor.size());
//     EXPECT_EQ(gscreen->screenPixmap()->toImage(), noCursor);
// }

// TEST(ErrorExplame, NoCursor) {
//     GoatScreenshotGDI gscreen;
//     EXPECT_TRUE(gscreen->screenPixmap()->isNull());
//     EXPECT_FALSE(gscreen->showCursor(true));
// }

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}