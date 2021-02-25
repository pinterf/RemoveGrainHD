#include "avisynth.h"
#include "planar.h"
#include <cstring>

#if 1
void  debug_printf(const char* format, ...);
#endif

const int   plane[3] = { PLANAR_Y, PLANAR_U, PLANAR_V };
const int   planeRGB[3] = { PLANAR_G, PLANAR_B, PLANAR_R };

int __stdcall PlanarAccess::YUV_GetPitch(VideoFrame* frame, int i)
{
  return  frame->GetPitch(plane[i]);
}

int __stdcall PlanarAccess::RGB_GetPitch(VideoFrame* frame, int i)
{
  return  frame->GetPitch(planeRGB[i]);
}

int __stdcall PlanarAccess::YUY2_GetPitch(VideoFrame* frame, int i)
{
  return  frame->GetPitch();
}

const BYTE* __stdcall PlanarAccess::YUV_GetReadPtr(VideoFrame* frame, int i)
{
  return  frame->GetReadPtr(plane[i]);
}

const BYTE* __stdcall PlanarAccess::RGB_GetReadPtr(VideoFrame* frame, int i)
{
  return  frame->GetReadPtr(planeRGB[i]);
}

const BYTE* __stdcall PlanarAccess::YUY2_GetReadPtr(VideoFrame* frame, int i)
{
  return  frame->GetReadPtr() + planeoffset[i];
}

BYTE* __stdcall PlanarAccess::YUV_GetWritePtr(VideoFrame* frame, int i)
{
  return  frame->GetWritePtr(plane[i]);
}

BYTE* __stdcall PlanarAccess::RGB_GetWritePtr(VideoFrame* frame, int i)
{
  return  frame->GetWritePtr(planeRGB[i]);
}

BYTE* __stdcall PlanarAccess::YUY2_GetWritePtr(VideoFrame* frame, int i)
{
  return  frame->GetWritePtr() + planeoffset[i];
}


PlanarAccess::PlanarAccess(const VideoInfo& vi, bool planar)
{
  width[1] = width[0] = vi.width;
  height[1] = height[0] = vi.height;

  // 'planar' hack YUY2 or RGB
  _GetPitch = &PlanarAccess::YUY2_GetPitch;
  _GetReadPtr = &PlanarAccess::YUY2_GetReadPtr;
  _GetWritePtr = &PlanarAccess::YUY2_GetWritePtr;

  if (vi.IsYUV())
  {
    if (vi.IsYUY2()) {
      width[1] = vi.width / 2;
    }
    else {
      width[1] = vi.IsY() ? 0 : vi.width >> vi.GetPlaneWidthSubsampling(PLANAR_U);
      height[1] = vi.IsY() ? 0 : vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U);
      _GetPitch = &PlanarAccess::YUV_GetPitch;
      _GetReadPtr = &PlanarAccess::YUV_GetReadPtr;
      _GetWritePtr = &PlanarAccess::YUV_GetWritePtr;
    }
    width[2] = width[1];
    height[2] = height[1];
  }
  else if (vi.IsRGB() && vi.IsPlanar()) {
    width[2] = width[1] = width[0];
    height[2] = height[1] = height[0];
    _GetPitch = &PlanarAccess::RGB_GetPitch;
    _GetReadPtr = &PlanarAccess::RGB_GetReadPtr;
    _GetWritePtr = &PlanarAccess::RGB_GetWritePtr;
  }
  else {
    // planar hacked packed RGB
    width[2] = width[1];
    height[2] = height[1];
  }

  planeoffset[0] = 0;
  planeoffset[1] = width[0];
  planeoffset[2] = planeoffset[1] + width[1];
  // number of planes -1
  planes = vi.IsY() ? 0 : 2;

  // planar: old YUY2 hack parameter
  if (!planar && (vi.IsYUY2() || (vi.IsRGB() && !vi.IsPlanar())))
  {
    // (packed RGB or YUY2)+!planar
    // though it is supported no longer
    // 'planar': Avisynth 2.5 hack to store planar data inside a YUY2 or RGB24
    planes = 0;
    width[0] = vi.RowSize();
  }
}

void BitBlt(uint8_t* dstp, int dst_pitch, const uint8_t* srcp, int src_pitch, int row_size, int height)
{
  if (!height || !row_size) return;
  if (height == 1 || (dst_pitch == src_pitch && src_pitch == row_size))
    memcpy(dstp, srcp, src_pitch * height);
  else
  {
    for (int y = height; y > 0; --y)
    {
      memcpy(dstp, srcp, row_size);
      dstp += dst_pitch;
      srcp += src_pitch;
    }
  }
}
