#include <QImage>
#include <QDebug>
void ImageRealloc(QImage &img, 
    int width, int height, 
    QImage::Format format)
{
    if(format == img.format() && img.width() == width && img.height() == height){
        return;
    }
    img = QImage(width, height, format);
}

void ImageBuffCopy(QImage &img, uchar* buffptr)
{
    uchar* dst = img.bits();
    memcpy(dst, buffptr, img.bytesPerLine() * img.height());
    return;
}


void DrawCursorMaskMono(QImage &dest, uchar* pCursorBuffer, int CursorlineBytes)
{
    uint32_t* pDesktopBits32 = (uint32_t*)dest.bits();
    for(int j = 0; j<dest.height(); j++)
    {
        for(int i = 0; i<dest.width(); i++)
        {
            uchar Mask = 0x80 >> (i % 8);
            uchar aval = pCursorBuffer[i/8 + CursorlineBytes*j];
            uchar xval = pCursorBuffer[i/8 + CursorlineBytes*(j + dest.height())];
            uchar AndMask = aval & Mask;
            uchar XorMask = xval & Mask;

            uint32_t AndMask32 = (AndMask) ? 0xFFFFFFFF : 0xFF000000;
            uint32_t XorMask32 = (XorMask) ? 0x00FFFFFF : 0x00000000;

            pDesktopBits32[j*dest.width() + i]  &= AndMask32;
            pDesktopBits32[j*dest.width() + i]  ^= XorMask32;
        }
    }
}


void DrawCursorMask(QImage &dest, uchar* pCursorBuffer, int CursorlineBytes)
{
    
	qDebug() << "DrawCursorMask";
    // if(dest.width() / CursorlineBytes == 8){
	//     qDebug() << "DrawCursorMaskMono";
    //     DrawCursorMaskMono(dest, pCursorBuffer, CursorlineBytes);
    //     return;
    // }
    uint32_t* pShapeBuffer32 = (uint32_t*)pCursorBuffer;
    uint32_t* pDesktopBits32 = (uint32_t*)dest.bits();
    for(int j = 0; j<dest.height(); j++)
    {
        for(int i = 0; i<dest.width(); i++)
        {
            // Set up mask
            uint32_t MaskVal = 0xFF000000 & pShapeBuffer32[i + (CursorlineBytes/4)*j];
            if (MaskVal)
            {
                // Mask was 0xFF
                pDesktopBits32[j*dest.width() + i] ^= pShapeBuffer32[i + (CursorlineBytes/4)*j];
                pDesktopBits32[j*dest.width() + i] |= 0xFF000000;
            }
            else
            {
                // Mask was 0x00 - replacing pixel
                pDesktopBits32[j*dest.width() + i] = pShapeBuffer32[i + (CursorlineBytes/4)*j];
            }
        }
    }
}