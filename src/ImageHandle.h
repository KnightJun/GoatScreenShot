#pragma once
#include <QImage>
void ImageRealloc(QImage &img, int width, int height, QImage::Format format);
void ImageBuffCopy(QImage &img, uchar* buffptr);
void DrawCursorMask(QImage &dest, uchar* pCursorBuffer, int CursorlineBytes);