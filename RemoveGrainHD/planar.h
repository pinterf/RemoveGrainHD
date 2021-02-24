#ifndef	PLANAR_H
#define	PLANAR_H

extern const int plane[];

class	PlanarAccess
{
private:
	int	planeoffset[3];
	int (__stdcall PlanarAccess::*_GetPitch)(VideoFrame *frame, int i);
	const BYTE* (__stdcall PlanarAccess::*_GetReadPtr)(VideoFrame *frame, int i);
	BYTE* (__stdcall PlanarAccess::*_GetWritePtr)(VideoFrame *frame, int i);

	int __stdcall YV12_GetPitch(VideoFrame *frame, int i);
	
	int __stdcall YUY2_GetPitch(VideoFrame *frame, int i);

	const BYTE *__stdcall YV12_GetReadPtr(VideoFrame *frame, int i);

	const BYTE *__stdcall YUY2_GetReadPtr(VideoFrame *frame, int i);

	BYTE *__stdcall YV12_GetWritePtr(VideoFrame *frame, int i);

	BYTE *__stdcall YUY2_GetWritePtr(VideoFrame *frame, int i);

public:
	int	width[3];
	int	height[3];
	int	planes;

	inline int GetPitch(PVideoFrame &frame, int i)
	{
		return	(this->*_GetPitch)(frame.operator ->(), i);
	}

	inline const BYTE * GetReadPtr(PVideoFrame &frame, int i)
	{
		return	(this->*_GetReadPtr)(frame.operator ->(), i);
	}

	inline BYTE * GetWritePtr(PVideoFrame &frame, int i)
	{
		return	(this->*_GetWritePtr)(frame.operator ->(), i);
	}

	PlanarAccess(const VideoInfo &vi, bool planar = true);
};

void BitBlt(uint8_t* dstp, int dst_pitch, const uint8_t* srcp, int src_pitch, int row_size, int height);

#endif // PLANAR_H