#define VC_EXTRALEAN 
#include <Windows.h>
#include "avisynth.h"
#include "planar.h"

#if	1
void	debug_printf(const char *format, ...);
#endif

const	int		plane[3] = {PLANAR_Y, PLANAR_U, PLANAR_V};

int __stdcall PlanarAccess::YV12_GetPitch(VideoFrame *frame, int i)
{
	return	frame->GetPitch(plane[i]);
}
	
int __stdcall PlanarAccess::YUY2_GetPitch(VideoFrame *frame, int i)
{
	return	frame->GetPitch();	
}

const BYTE *__stdcall PlanarAccess::YV12_GetReadPtr(VideoFrame *frame, int i)
{
	return	frame->GetReadPtr(plane[i]);
}

const BYTE *__stdcall PlanarAccess::YUY2_GetReadPtr(VideoFrame *frame, int i)
{
	return	frame->GetReadPtr() + planeoffset[i];
}

BYTE *__stdcall PlanarAccess::YV12_GetWritePtr(VideoFrame *frame, int i)
{
	return	frame->GetWritePtr(plane[i]);
}

BYTE *__stdcall PlanarAccess::YUY2_GetWritePtr(VideoFrame *frame, int i)
{
	return	frame->GetWritePtr() + planeoffset[i];
}


PlanarAccess::PlanarAccess(const VideoInfo &vi, bool planar)
{
	_GetPitch = &PlanarAccess::YUY2_GetPitch;
	_GetReadPtr = &PlanarAccess::YUY2_GetReadPtr;
	_GetWritePtr = &PlanarAccess::YUY2_GetWritePtr;
	width[1] = width[0] = vi.width;
	height[1] = height[0] = vi.height;
	if( vi.IsYUV() )
	{
		width[1] /= 2;
		if( vi.IsYV12() )
		{
			height[1] /= 2;
			_GetPitch = &PlanarAccess::YV12_GetPitch;
			_GetReadPtr = &PlanarAccess::YV12_GetReadPtr;
			_GetWritePtr = &PlanarAccess::YV12_GetWritePtr;
		}
	}
	width[2] = width[1];
	height[2] = height[1];
	planeoffset[0] = 0;
	planeoffset[1] = width[0];
	planeoffset[2] = planeoffset[1] + width[1];
	planes = 2;

	if( planar + vi.IsYV12() == 0 ) 
	{
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
