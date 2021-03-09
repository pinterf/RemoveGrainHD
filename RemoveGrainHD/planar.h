#ifndef PLANAR_H
#define PLANAR_H

extern const int plane[];

class PlanarAccess
{
private:
  int planeoffset[3]; // YUY2 planar
  int(__stdcall PlanarAccess::* _GetPitch)(VideoFrame* frame, int i);
  const BYTE* (__stdcall PlanarAccess::* _GetReadPtr)(VideoFrame* frame, int i);
  BYTE* (__stdcall PlanarAccess::* _GetWritePtr)(VideoFrame* frame, int i);

  int __stdcall YUV_GetPitch(VideoFrame* frame, int i);
  int __stdcall RGB_GetPitch(VideoFrame* frame, int i);
  int __stdcall YUY2_GetPitch(VideoFrame* frame, int i);
  const BYTE* __stdcall YUV_GetReadPtr(VideoFrame* frame, int i);
  const BYTE* __stdcall RGB_GetReadPtr(VideoFrame* frame, int i);
  const BYTE* __stdcall YUY2_GetReadPtr(VideoFrame* frame, int i);
  BYTE* __stdcall YUV_GetWritePtr(VideoFrame* frame, int i);
  BYTE* __stdcall RGB_GetWritePtr(VideoFrame* frame, int i);
  BYTE* __stdcall YUY2_GetWritePtr(VideoFrame* frame, int i);

public:
  int width[3];
  int height[3];
  int planes;

  inline int GetPitch(PVideoFrame& frame, int i)
  {
    return  (this->*_GetPitch)(frame.operator ->(), i);
  }

  inline const BYTE* GetReadPtr(PVideoFrame& frame, int i)
  {
    return  (this->*_GetReadPtr)(frame.operator ->(), i);
  }

  inline BYTE* GetWritePtr(PVideoFrame& frame, int i)
  {
    return  (this->*_GetWritePtr)(frame.operator ->(), i);
  }

  PlanarAccess(const VideoInfo& vi, bool planar);
};

void BitBlt(uint8_t* dstp, int dst_pitch, const uint8_t* srcp, int src_pitch, int row_size, int height);

#endif // PLANAR_H