#include "GoatScreenshotDXGI.h"
#include "GoatScreenshotGDI.h"
#include <QScreen>
#include <QtDebug>
#include <QMessageBox>
#include <QApplication>

#include <Windows.h>
#include <utilapiset.h>

#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgi.h>

#include <dxgi.h>
#include <d3d11.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <Psapi.h>
#include "ImageHandle.h"
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"D3D11.lib")
#pragma comment(lib,"Shcore.lib")
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"windowscodecs.lib")
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "dxguid.lib")

#define LEN(e) (sizeof(e)/sizeof(e[0]))
void ShowError(QString error)
{
    QMessageBox msg;
    msg.setText(error);
    msg.exec();

    QApplication::exit(-1); //abort();
}

typedef struct _PTR_INFO
{
    _Field_size_bytes_(BufferSize) BYTE* PtrShapeBuffer = nullptr;
    DXGI_OUTDUPL_POINTER_SHAPE_INFO ShapeInfo;
    POINT Position;
    bool Visible;
    UINT BufferSize = 0;
    UINT WhoUpdatedPositionLast = 0;
    LARGE_INTEGER LastTimeStamp;
} PTR_INFO;

struct DXGIParams
{
    ID3D11Device *device=nullptr;
    ID3D11DeviceContext *device_context=nullptr;
    IDXGIOutputDuplication *output_duplication=nullptr;
    ID3D11Texture2D *capture_texture=nullptr;
    ID3D11Texture2D *region_copy_texture=nullptr;
    IDXGISurface *region_copy_surface=nullptr;
};

GoatScreenshotDXGI::~GoatScreenshotDXGI()
{
    if(mMetaDataBuffer) delete mMetaDataBuffer;
    if(DXGI->device) DXGI->device->Release();
    if(DXGI->device_context) DXGI->device_context->Release();
    if(DXGI->output_duplication) DXGI->output_duplication->Release();
    if(DXGI->capture_texture) DXGI->capture_texture->Release();
    if(DXGI->region_copy_surface) DXGI->region_copy_surface->Release();
    if(DXGI->region_copy_texture) DXGI->region_copy_texture->Release();
    delete DXGI;
    delete mPtrInfo;
}

GoatScreenshotDXGI::GoatScreenshotDXGI(QRect shotRect, QScreen *screen, bool withCursor)
	:GoatScreenshot(shotRect, screen, withCursor)
{
    DXGI = new DXGIParams;
    mPtrInfo = new PTR_INFO;
    IDXGIFactory1 *dxgi_factory = nullptr;
    HRESULT hr = CreateDXGIFactory1(IID_IDXGIFactory1, 
        reinterpret_cast<void**>(&dxgi_factory));
    if (FAILED(hr))
    {
        ShowError(QStringLiteral("Error create dxgi factory: %1").arg(hr));
    }

    D3D_FEATURE_LEVEL supported_feature_levels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    D3D_FEATURE_LEVEL fl;

    hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, 
        nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, 
        supported_feature_levels, LEN(supported_feature_levels), 
        D3D11_SDK_VERSION, &DXGI->device, 
        &fl, &DXGI->device_context);

    if (FAILED(hr))
    {
        ShowError(QStringLiteral("Error create d3d11 device: %1").arg(hr));

    }

    // find the display that has the window on it.
    IDXGIAdapter1 *adapter;
    for (uint adapter_index = 0; 
        dxgi_factory->EnumAdapters1(adapter_index, &adapter) 
            != DXGI_ERROR_NOT_FOUND; 
        adapter_index++)
    {
        // enumerate outputs
        IDXGIOutput *output;
        for (uint output_index = 0; adapter->EnumOutputs(output_index, &output) != DXGI_ERROR_NOT_FOUND; output_index++)
        {
            DXGI_OUTPUT_DESC output_desc;
            output->GetDesc(&output_desc);
            qInfo() << "Display output found"
            << ", DeviceName=" << output_desc.DeviceName
            << ", AttachedToDesktop=" << output_desc.AttachedToDesktop
            << ", Rotation=" << output_desc.Rotation
            << ", DesktopCoordinates="
            << QString("{(%1,%2),(%3,%4)}")
            .arg(output_desc.DesktopCoordinates.left)
            .arg(output_desc.DesktopCoordinates.top)
            .arg(output_desc.DesktopCoordinates.right)
            .arg(output_desc.DesktopCoordinates.bottom);
            if (output_desc.AttachedToDesktop 
                && mScreen->name() == QString::fromWCharArray(output_desc.DeviceName))
            {
                IDXGIOutput1 *output1 = static_cast<IDXGIOutput1*>(output);
                hr = output1->DuplicateOutput(static_cast<IUnknown *>(DXGI->device), &DXGI->output_duplication);
                Sleep(100);
                if (FAILED(hr))
                {
                    ShowError(QStringLiteral("Output Duplication Failed: %1").arg(hr));
                }
                qInfo() << "Found.";
            }
            output->Release();
        }
        adapter->Release();
    }
    dxgi_factory->Release();
}

int GetMouse(DXGI_OUTDUPL_FRAME_INFO *FrameInfo, 
    PTR_INFO* PtrInfo, QRect shotRect, DXGIParams *DXGI,
    int OutputNumber)
{
    // A non-zero mouse update timestamp indicates that there is a mouse position update and optionally a shape change
    if (FrameInfo->LastMouseUpdateTime.QuadPart == 0)
    {
        return -1;
    }

    bool UpdatePosition = true;

    // Make sure we don't update pointer position wrongly
    // If pointer is invisible, make sure we did not get an update from another output that the last time that said pointer
    // was visible, if so, don't set it to invisible or update.
    if (!FrameInfo->PointerPosition.Visible && 
        (PtrInfo->WhoUpdatedPositionLast != OutputNumber))
    {
        UpdatePosition = false;
    }

    // If two outputs both say they have a visible, only update if new update has newer timestamp
    if (FrameInfo->PointerPosition.Visible 
        && PtrInfo->Visible 
        && (PtrInfo->WhoUpdatedPositionLast != OutputNumber) 
        && (PtrInfo->LastTimeStamp.QuadPart > FrameInfo->LastMouseUpdateTime.QuadPart))
    {
        UpdatePosition = false;
    }

    // Update position
    if (UpdatePosition)
    {
        PtrInfo->Position.x = FrameInfo->PointerPosition.Position.x - shotRect.x();
        PtrInfo->Position.y = FrameInfo->PointerPosition.Position.y- shotRect.y();
        PtrInfo->WhoUpdatedPositionLast = OutputNumber;
        PtrInfo->LastTimeStamp = FrameInfo->LastMouseUpdateTime;
        PtrInfo->Visible = FrameInfo->PointerPosition.Visible != 0;
    }

    // No new shape
    if (FrameInfo->PointerShapeBufferSize == 0)
    {
        return 0;
    }

    // Old buffer too small
    if (FrameInfo->PointerShapeBufferSize > PtrInfo->BufferSize)
    {
        if (PtrInfo->PtrShapeBuffer)
        {
            delete [] PtrInfo->PtrShapeBuffer;
            PtrInfo->PtrShapeBuffer = nullptr;
        }
        PtrInfo->PtrShapeBuffer = new (std::nothrow) BYTE[FrameInfo->PointerShapeBufferSize];
        if (!PtrInfo->PtrShapeBuffer)
        {
            PtrInfo->BufferSize = 0;
            ShowError("Failed to allocate memory for pointer shape in DUPLICATIONMANAGER");
            return -1;
        }

        // Update buffer size
        PtrInfo->BufferSize = FrameInfo->PointerShapeBufferSize;
    }

    // Get shape
    UINT BufferSizeRequired;
    HRESULT hr = DXGI->output_duplication->GetFramePointerShape(
        FrameInfo->PointerShapeBufferSize, 
        reinterpret_cast<VOID*>(PtrInfo->PtrShapeBuffer), 
        &BufferSizeRequired, &(PtrInfo->ShapeInfo));
    if (FAILED(hr))
    {
        delete [] PtrInfo->PtrShapeBuffer;
        PtrInfo->PtrShapeBuffer = nullptr;
        PtrInfo->BufferSize = 0;
        ShowError("Failed to get frame pointer shape in DUPLICATIONMANAGER");
        return -1;
    }
    return 0;
}

int DrawMouse(GoatScreenshotDXGI* goatShot)
{
    PTR_INFO* PtrInfo = goatShot->mPtrInfo;
	QRect rcPointer(PtrInfo->Position.x, PtrInfo->Position.y, 
                PtrInfo->ShapeInfo.Width, PtrInfo->ShapeInfo.Height);
    goatShot->mMouseImagePoint = rcPointer.topLeft();
    if(PtrInfo->ShapeInfo.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME)
    {
        rcPointer.setHeight(PtrInfo->ShapeInfo.Height / 2);
    }
    if(!rcPointer.intersects(QRect(0, 0, 
        goatShot->mShotRect.width(), goatShot->mShotRect.height()))){
        goatShot->mHasMouse = false;
		return -1;
	}
    goatShot->mHasMouse = true;
    ImageRealloc(goatShot->mMouseImage, 
        rcPointer.width(), rcPointer.height(), 
        QImage::Format_ARGB32);
    switch (PtrInfo->ShapeInfo.Type)
    {
    case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR:
    {
        ImageBuffCopy(goatShot->mMouseImage, PtrInfo->PtrShapeBuffer);
        break;
    }
    case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR:
    case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME:
    {
        goatShot->mMouseImage = goatShot->mScreenImage.copy(rcPointer);
        DrawCursorMask(goatShot->mMouseImage, 
            PtrInfo->PtrShapeBuffer, PtrInfo->ShapeInfo.Pitch);
        break;
    }
    default:
        break;
    }
    return 0;
}

bool GoatScreenshotDXGI::getNextFrame()
{
    DXGI_OUTDUPL_FRAME_INFO capture_frame_info;
    IDXGIResource *resource;
    HRESULT hr = S_OK;
    uint DirtyCount;
    uint MoveCount;
    uint BufSize;
    int getMouseRet = 0;
    hr = DXGI->output_duplication->AcquireNextFrame(0, &capture_frame_info, &resource);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT){
        return false;
    }
    if (FAILED(hr))
    {
        ShowError("AcquireNextFrame Fail");
        return false;
    }
    if (capture_frame_info.TotalMetadataBufferSize > mMetaDataSize){
        if(mMetaDataBuffer) delete mMetaDataBuffer;
        mMetaDataBuffer = new BYTE[capture_frame_info.TotalMetadataBufferSize];
        mMetaDataSize = capture_frame_info.TotalMetadataBufferSize;
    }
    BufSize = capture_frame_info.TotalMetadataBufferSize;
    if(BufSize){
        hr = DXGI->output_duplication->GetFrameMoveRects(BufSize, 
            reinterpret_cast<DXGI_OUTDUPL_MOVE_RECT*>(mMetaDataBuffer), &BufSize);
        if (FAILED(hr))
        {
            ShowError("GetFrameMoveRects Fail");
            return false;
        }
        MoveCount = BufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);
        BYTE* DirtyRects = mMetaDataBuffer + BufSize;
        BufSize = capture_frame_info.TotalMetadataBufferSize - BufSize;
        // Get dirty rectangles
        hr = DXGI->output_duplication->GetFrameDirtyRects(BufSize, 
            reinterpret_cast<RECT*>(DirtyRects), &BufSize);
        if (FAILED(hr))
        {
            ShowError("GetFrameDirtyRects fail ");
            return false;
        }
        DirtyCount = BufSize / sizeof(RECT);
    }
    mOutputNumber++;
    resource->QueryInterface(IID_ID3D11Texture2D, 
        reinterpret_cast<void**>(&DXGI->capture_texture));
    resource->Release();

    if (DXGI->capture_texture)
    {
        //if (verbosity)qDebug()<<"texture captured ok\n";
        D3D11_TEXTURE2D_DESC capture_texture_desc;
        DXGI->capture_texture->GetDesc(&capture_texture_desc);
        if(!DXGI->region_copy_texture){
            D3D11_TEXTURE2D_DESC region_texture_desc;
            ZeroMemory(&region_texture_desc, sizeof(region_texture_desc));
            region_texture_desc.Width = capture_texture_desc.Width ;
            region_texture_desc.Height = capture_texture_desc.Height;
            region_texture_desc.MipLevels = 1;
            region_texture_desc.ArraySize = 1;
            region_texture_desc.SampleDesc.Count = 1;
            region_texture_desc.SampleDesc.Quality = 0;
            region_texture_desc.Usage = D3D11_USAGE_STAGING;
            region_texture_desc.Format = capture_texture_desc.Format;
            region_texture_desc.BindFlags = 0;
            region_texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            region_texture_desc.MiscFlags = 0;
            hr = DXGI->device->CreateTexture2D(&region_texture_desc, 
                nullptr, &DXGI->region_copy_texture);
            if (FAILED(hr))
            {
                ShowError("error - CreateTexture2d ");
                return false;
            }
            DXGI->region_copy_texture->QueryInterface(IID_IDXGISurface, 
                reinterpret_cast<void**>(&DXGI->region_copy_surface));
        }
        
        DXGI->device_context->CopyResource(
            static_cast<ID3D11Resource *>(DXGI->region_copy_texture),
            static_cast<ID3D11Resource *>(DXGI->capture_texture));

        DXGI_MAPPED_RECT rect;
        hr = DXGI->region_copy_surface->Map(&rect, DXGI_MAP_READ);
        if (FAILED(hr))
        {
            DXGI->region_copy_surface->Unmap();
            DXGI->region_copy_surface->Release();
            DXGI->region_copy_texture->Release();
            ShowError("error - region_copy_surface:");
            return false;
        }

        QImage img = QImage(rect.pBits, capture_texture_desc.Width, capture_texture_desc.Height, QImage::Format_RGB32);
        if(capture_texture_desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM){
            mScreenImage = img.copy(mShotRect);
        }else if(capture_texture_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM){
            mScreenImage = img.copy(mShotRect).rgbSwapped();
        }else{
            ShowError(QString("unknow format:%1").arg(capture_texture_desc.Format));
        }
        
        if(mWithCursor){
            getMouseRet = GetMouse(&capture_frame_info, mPtrInfo, mShotRect, DXGI, 0);
            if(getMouseRet == -1){
                mHasMouse = GoatScreenshotGDI::getMouseImage(mShotRect, mScreenImage, mMouseImage, mMouseImagePoint);
            }else{
                mHasMouse = (DrawMouse(this) == 0);
            }
        }

        DXGI->capture_texture->Release();
        DXGI->capture_texture = nullptr;
        DXGI->region_copy_surface->Unmap();
        DXGI->output_duplication->ReleaseFrame();
        return true;
    }
    return false;
}