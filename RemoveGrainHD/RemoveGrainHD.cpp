#define LOGO		"RemoveGrainHD 0.5\n"
// An Avisynth plugin for removing grain from progressive video
//
// By Rainer Wittmann <gorw@gmx.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// To get a copy of the GNU General Public License write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .

#define VC_EXTRALEAN 
#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>

#define	DEFAULT_RADIUS 2
//#define DEBUG_NAME
//#define	PROFILE_VERSION
#define SMARTMEDIAN_ODD	
#define	LOOKUP
#define	LOCALTABLE
//#define AVS_COPY
#define	STEP	1
#define	DEBUG_PRINTF_LIMIT	20


#define DPRINTF_SIZE	200

#ifdef	DEBUG_PRINTF_LIMIT
static int	debug_printf_count = DEBUG_PRINTF_LIMIT;
#endif

static	void	debug_printf(const char *format, ...)
{
#ifdef	DEBUG_PRINTF_LIMIT
	if( --debug_printf_count >= 0 )
#endif
	{
		char	buffer[DPRINTF_SIZE];
		va_list	args;
		va_start(args, format);
		vsprintf_s(buffer, DPRINTF_SIZE, format, args);
		va_end(args);
		OutputDebugString(buffer);
	}
}

#include "avisynth.h"
#include "planar.h"

#define TABLE_MAX	255

#ifdef	ADD_LINE
#undef  ADD_LINE
#undef  SUBTRACT_LINE
#undef  ADD_COLUMN
#undef  SUBTRACT_COLUMN
#endif
#ifdef	LOOKUP
#define	ADD_LINE()					\
{									\
	int j = xr;						\
	do								\
	{								\
		int	k = *src++;				\
		sum += lookup[k];			\
		table[k]++;					\
	} while( --j );					\
	src += incpitch;				\
}

#define SUBTRACT_LINE()				\
{									\
	int j = xr;						\
	do								\
	{								\
		int k = *last++;			\
		table[k]--;					\
		sum -= lookup[k];			\
	} while( --j );					\
	last += incpitch;				\
}

#define ADD_COLUMN()				\
{									\
	src -= incpitch;				\
	int j = yr;						\
	do								\
	{								\
		int	k = *src;				\
		sum += lookup[k];			\
		table[k]++;					\
		src += spitch;				\
	} while( --j );					\
}

#define SUBTRACT_COLUMN()			\
{									\
	int j = yr;						\
	do								\
	{								\
		int	k = *last;				\
		sum -= lookup[k];			\
		table[k]--;					\
		last += spitch;				\
	} while( --j );					\
	last -= (spitch - STEP);			\
}
#else	// LOOKUP
#define	ADD_LINE()					\
{									\
	int j = xr;						\
	do								\
	{								\
		int	k = *src++;				\
		if( k <= lval ) ++sum;		\
		table[k]++;					\
	} while( --j );					\
	src += incpitch;				\
}

#define SUBTRACT_LINE()				\
{									\
	int j = xr;						\
	do								\
	{								\
		int k = *last++;			\
		table[k]--;					\
		if( k <= lval ) --sum;		\
	} while( --j );					\
	last += incpitch;				\
}

#define ADD_COLUMN()				\
{									\
	src -= incpitch;				\
	int j = yr;						\
	do								\
	{								\
		int	k = *src;				\
		if( k <= lval ) ++sum;		\
		table[k]++;					\
		src += spitch;				\
	} while( --j );					\
}

#define SUBTRACT_COLUMN()			\
{									\
	int j = yr;						\
	do								\
	{								\
		int	k = *last;				\
		if( k <= lval ) --sum;		\
		table[k]--;					\
		last += spitch;				\
	} while( --j );					\
	last -= (spitch - STEP);			\
}
#endif	// LOOKUP

#ifdef	TABLE_SIZE
#undef	TABLE_SIZE
#endif
#define	TABLE_SIZE	(TABLE_MAX + 1)

#ifdef	check_table
#undef	check_table
#endif
#define check_table(pval)										\
{																\
	int	sum2 = 0;												\
	i = 0;														\
	while(i <= lval)											\
	{															\
		if(table[i] < 0) debug_printf("negativ value 1\n");		\
		sum2 += table[i++];										\
	}															\
	if( sum2 != sum + limit ) debug_printf("incorrect sum %i, %i, lval = %i\n", sum2, sum + limit, lval);	\
	while(i <= TABLE_MAX)										\
	{															\
		if( table[i] < 0 ) debug_printf("negativ value 2\n");	\
		sum2 += table[i++];										\
	}															\
	if(sum2 != xr * yr) debug_printf("incorrect table sum\n");	\
}
	

#ifdef	LOOKUP
#ifdef	PROFILE_VERSION
#define csquantile1(inc, x, y)							\
		dst += inc;										\
		sum += limit;									\
		limit = limits[x][y];							\
		if( (sum -= limit) < 0 )						\
		{												\
			int olval = lval;							\
			do {} while( (sum += table[++lval]) < 0 );	\
			profile += (lval - olval);					\
			dst[0] = lval;								\
		}												\
		else											\
		{												\
			int olval = lval;							\
			do {} while( (sum -= table[lval--]) >= 0 );	\
			profile += (olval - lval);					\
			dst[0] = lval + 1;							\
		}												\
		lookup = ltable + TABLE_MAX - lval;

#define csquantile2(inc)								\
		dst += inc;										\
		if( sum < 0 )									\
		{												\
			int olval = lval;							\
			do {} while( (sum += table[++lval]) < 0 );	\
			profile += (lval - olval);					\
			dst[0] = lval;								\
		}												\
		else											\
		{												\
			int olval = lval;							\
			do {} while( (sum -= table[lval--]) >= 0 );	\
			profile += (olval - lval);					\
			dst[0] = lval + 1;							\
		}												\
		lookup = ltable + TABLE_MAX - lval;
#else // PROFILE_VERSION
#define csquantile1(inc, x, y)							\
		dst += inc;										\
		sum += limit;									\
		limit = limits[x][y];							\
		if( (sum -= limit) < 0 )						\
		{												\
			do {} while( (sum += table[++lval]) < 0 );	\
			dst[0] = lval;								\
		}												\
		else											\
		{												\
			do {} while( (sum -= table[lval--]) >= 0 );	\
			dst[0] = lval + 1;							\
		}												\
		lookup = ltable + TABLE_MAX - lval;

#define csquantile2(inc)								\
		dst += inc;										\
		if( sum < 0 )									\
		{												\
			do {} while( (sum += table[++lval]) < 0 );	\
			dst[0] = lval;								\
		}												\
		else											\
		{												\
			do {} while( (sum -= table[lval--]) >= 0 );	\
			dst[0] = lval + 1;							\
		}												\
		lookup = ltable + TABLE_MAX - lval;
#endif	// PROFILE_VERSION
#else
#define csquantile1(inc, x, y)							\
		dst += inc;										\
		sum += limit;									\
		limit = limits[x][y];							\
		if( (sum -= limit) < 0 )						\
		{												\
			do {} while( (sum += table[++lval]) < 0 );	\
			dst[0] = lval;								\
		}												\
		else											\
		{												\
			do {} while( (sum -= table[lval--]) >= 0 );	\
			dst[0] = lval + 1;							\
		}

#define csquantile2(inc)								\
		dst += inc;										\
		if( sum < 0 )									\
		{												\
			do {} while( (sum += table[++lval]) < 0 );	\
			dst[0] = lval;								\
		}												\
		else											\
		{												\
			do {} while( (sum -= table[lval--]) >= 0 );	\
			dst[0] = lval + 1;							\
		}
#endif

#ifdef	PROCESS_TABLE1
#undef	PROCESS_TABLE1
#undef	PROCESS_TABLE2
#endif
#define	PROCESS_TABLE1(dinc, sinc, label)		csquantile2(dinc)
#define	PROCESS_TABLE2(dinc, sinc, x, y, label)	csquantile1(dinc, x, y)

#ifdef	INIT_FUNC
#undef	INIT_FUNC
#endif
#define INIT_FUNC()	int limit = 0

#ifdef	LOOKUP
int		ltable[2 * TABLE_MAX + 2]; // +2 instead of +1 is necessary, because lval=-1 may happen
#endif

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif
		single_quantile(BYTE *dst, int dpitch, const BYTE *src, int spitch, int w, int h, int **limits, int xradius, int yradius
#ifdef	LOCALTABLE
						)
#else						
						, int corner_boxsize, int *table)
#endif
{
#		include "skeleton.h"
}

#ifdef	check_table
#undef	check_table
#endif
#define check_table(pval)										\
{																\
	int	sum2 = 0;												\
	i = 0;														\
	while(i <= lval)											\
	{															\
		if(table[i] < 0) debug_printf("negativ value 1\n");		\
		sum2 += table[i++];										\
	}															\
	if( sum2 != sum + llimit ) debug_printf("incorrect sum %i, %i, %i, %i\n", sum2, sum + llimit, ulimit, pval);	\
	while(i <= TABLE_MAX)										\
	{															\
		if( table[i] < 0 ) debug_printf("negativ value 2\n");	\
		sum2 += table[i++];										\
	}															\
	if(sum2 != xr * yr) debug_printf("incorrect table sum, %i, %i, %i\n", sum2, xr, yr);\
}

#ifdef	PROFILE_VERSION
#define RG2(inc, label)								\
{													\
	int	val = (dst += inc)[0];						\
	if( sum < 0 )									\
	{												\
		do { ++profile; } while( (sum += table[++lval]) < 0 );	\
		if( val <= lval ) goto setval##label;		\
		if( (sum -= ulimit) > 0 )					\
			goto setvalrestsum##label;				\
		below##label:								\
		do											\
		{											\
			 ++profile;								\
			if( (sum += table[++lval]) > 0 )		\
				goto setvalrestsum##label;			\
		} while( lval != val );						\
		sum += ulimit;								\
		goto do_nothing##label;						\
		setvalrestsum##label:						\
		sum += ulimit;								\
		setval##label:								\
		dst[0] = lval;								\
		goto do_nothing##label;						\
	}												\
	if( (sum -= ulimit) > 0 )						\
	{ 												\
		do { ++profile; } while( (sum -= table[lval--]) > 0 );	\
		if( val > lval )							\
		{											\
			sum += ulimit;							\
			dst[0] = lval + 1;						\
			goto do_nothing##label;					\
		}											\
		if( (sum += ulimit) < 0 )					\
			goto setvaln##label;					\
		/* lval >= val */							\
		do											\
		{											\
			++profile;								\
			if( val == lval )						\
				goto do_nothing##label;				\
		} while( (sum -= table[lval--]) >= 0 );		\
		setvaln##label:								\
		dst[0] = lval + 1;							\
		goto do_nothing##label;						\
	}												\
	/* lval is now within the RG limits */			\
	/* sum -= ulimit */								\
	if( lval < val ) goto below##label;				\
	sum += ulimit;									\
	if( lval > val )								\
	{												\
		do											\
		{											\
			++profile;								\
			if( (sum -= table[lval--]) < 0 )		\
				goto setvaln##label;				\
		} while( lval != val );						\
	}												\
	/* lval == val */								\
	do_nothing##label:								\
	lookup = ltable + TABLE_MAX - lval;				\
}

#else // PROFILE_VERSION
//#define RG_DEBUG
#ifdef	RG_DEBUG
#define RG2(inc, label)								\
{													\
	int	val = (dst += inc)[0];						\
	if( sum < 0 )									\
	{	state = 0;									\
		do {} while( (sum += table[++lval]) < 0 );	\
		if( val <= lval ) goto setval##label;		\
		if( (sum -= ulimit) > 0 )					\
			goto setvalrestsum##label;				\
		below##label:								\
		do											\
		{											\
			if( (sum += table[++lval]) > 0 )		\
				goto setvalrestsum##label;			\
		} while( lval != val );						\
		sum += ulimit;								\
		goto do_nothing##label;						\
		setvalrestsum##label:						\
		sum += ulimit;								\
		setval##label:								\
		dst[0] = lval;								\
		goto do_nothing##label;						\
	}												\
	if( (sum -= ulimit) > 0 )						\
	{ 	state = 2;									\
		do {} while( (sum -= table[lval--]) > 0 );	\
		if( val > lval )							\
		{											\
			sum += ulimit;							\
			dst[0] = lval + 1;						\
			goto do_nothing##label;					\
		}											\
		if( (sum += ulimit) < 0 )					\
			goto setvaln##label;					\
		/* lval >= val */							\
		do											\
		{											\
			if( val == lval )						\
				goto do_nothing##label;				\
		} while( (sum -= table[lval--]) >= 0 );		\
		setvaln##label:								\
		dst[0] = lval + 1;							\
		goto do_nothing##label;						\
	}												\
	/* lval is now within the RG limits */			\
	/* sum -= ulimit */								\
	if( lval < val ) goto below##label;				\
	sum += ulimit;									\
	if( lval > val )								\
	{	state = 1;									\
		do											\
		{											\
			if( (sum -= table[lval--]) < 0 )		\
				goto setvaln##label;				\
		} while( lval != val );	state = 5;			\
	}												\
	/* lval == val */								\
	do_nothing##label:								\
	lookup = ltable + TABLE_MAX - lval;				\
	/* if( dst == dst0 + 1*dpitch0 + 277 ) debug_printf("%i, %i\n", dst[0], state); */\
	/* if( val != dst[0] ) debug_printf("%i, %i, %i, %i, %i, %i, %i\n", state, val, dst[0], llimit, ulimit, xr, yr);*/\
}
#else // RG_DEBUG
#define RG2(inc, label)								\
{													\
	int	val = (dst += inc)[0];						\
	if( sum < 0 )									\
	{												\
		do {} while( (sum += table[++lval]) < 0 );	\
		if( val <= lval ) goto setval##label;		\
		if( (sum -= ulimit) > 0 )					\
			goto setvalrestsum##label;				\
		below##label:								\
		do											\
		{											\
			if( (sum += table[++lval]) > 0 )		\
				goto setvalrestsum##label;			\
		} while( lval != val );						\
		sum += ulimit;								\
		goto do_nothing##label;						\
		setvalrestsum##label:						\
		sum += ulimit;								\
		setval##label:								\
		dst[0] = lval;								\
		goto do_nothing##label;						\
	}												\
	if( (sum -= ulimit) > 0 )						\
	{ 												\
		do {} while( (sum -= table[lval--]) > 0 );	\
		if( val > lval )							\
		{											\
			sum += ulimit;							\
			dst[0] = lval + 1;						\
			goto do_nothing##label;					\
		}											\
		if( (sum += ulimit) < 0 )					\
			goto setvaln##label;					\
		/* lval >= val */							\
		do											\
		{											\
			if( val == lval )						\
				goto do_nothing##label;				\
		} while( (sum -= table[lval--]) >= 0 );		\
		setvaln##label:								\
		dst[0] = lval + 1;							\
		goto do_nothing##label;						\
	}												\
	/* lval is now within the RG limits */			\
	/* sum -= ulimit */								\
	if( lval < val ) goto below##label;				\
	sum += ulimit;									\
	if( lval > val )								\
	{												\
		do											\
		{											\
			if( (sum -= table[lval--]) < 0 )		\
				goto setvaln##label;				\
		} while( lval != val );						\
	}												\
	/* lval == val */								\
	do_nothing##label:								\
	lookup = ltable + TABLE_MAX - lval;				\
}
#endif	// RG_DEBUG
#endif	// PROFILE_VERSION

#define RG1(inc, x, y, label)						\
	sum += llimit;									\
	ulimit = ulimits[x][y];							\
	sum -= (llimit = llimits[x][y]);				\
	RG2(inc, label)

#ifdef	PROCESS_TABLE1
#undef	PROCESS_TABLE1
#undef	PROCESS_TABLE2
#endif
#define	PROCESS_TABLE1(dinc, sinc, label)		RG2(dinc, label)
#define	PROCESS_TABLE2(dinc, sinc, x, y, label)	RG1(dinc, x, y, label)

#ifdef	INIT_FUNC
#undef	INIT_FUNC
#endif

#ifdef	RG_DEBUG
#define INIT_FUNC()				\
	int ulimit, llimit = 0;		\
	int	state;					\
	int dpitch0 = dpitch;		\
	BYTE *dst0 = dst;
#else
#define INIT_FUNC()	int ulimit, llimit = 0
#endif

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif
		removegrain(BYTE *dst, int dpitch, const BYTE *src, int spitch, int w, int h, int **llimits, int **ulimits, int xradius, int yradius
#ifdef	LOCALTABLE
						)
#else						
						, int corner_boxsize, int *table)
#endif
{
#		include "skeleton.h"
}


#ifdef	check_table
#undef	check_table
#endif
#define check_table(pval)										\
{																\
	int	sum2 = 0;												\
	i = 0;														\
	while(i <= lval)											\
	{															\
	if(table[i] < 0) debug_printf("negativ value %i, %i, %i, %i\n", lval, i, table[i], pval);	\
		sum2 += table[i++];										\
	}															\
	if( sum2 != sum + llimit ) debug_printf("incorrect sum %i, %i, %i, %i\n", sum2, sum + llimit, ulimit, pval);	\
	while(i <= TABLE_MAX)										\
	{															\
		if( table[i] < 0 ) debug_printf("negativ value %i, %i, %i, %i\n", lval, i, table[i], pval);	\
		sum2 += table[i++];										\
	}															\
	if(sum2 != xr * yr * (2 + weight)) debug_printf("incorrect table sum, %i, %i, %i\n", sum2, xr, yr);\
}

#ifndef	LOOKUP
#define	LOOKUP
#endif

#ifdef	ADD_LINE
#undef  ADD_LINE
#undef  SUBTRACT_LINE
#undef  ADD_COLUMN
#undef  SUBTRACT_COLUMN
#endif
#define	ADD_LINE()					\
{									\
	int j = xr;						\
	do								\
	{								\
		int	k = *src0++;			\
		sum += wlookup[k];			\
		table[k] += weight;			\
	} while( --j );					\
	src0 += incpitch0;				\
									\
	j = xr;							\
	do								\
	{								\
		int	k = *src1++;			\
		sum += lookup[k];			\
		table[k]++;					\
	} while( --j );					\
	src1 += incpitch1;				\
									\
	j = xr;							\
	do								\
	{								\
		int	k = *src2++;			\
		sum += lookup[k];			\
		table[k]++;					\
	} while( --j );					\
	src2 += incpitch2;				\
}

#define SUBTRACT_LINE()				\
{									\
	int j = xr;						\
	do								\
	{								\
		int k = *last0++;			\
		table[k] -= weight;			\
		sum -= wlookup[k];			\
	} while( --j );					\
	last0 += incpitch0;				\
									\
	j = xr;							\
	do								\
	{								\
		int k = *last1++;			\
		table[k]--;					\
		sum -= lookup[k];			\
	} while( --j );					\
	last1 += incpitch1;				\
									\
	j = xr;							\
	do								\
	{								\
		int k = *last2++;			\
		table[k]--;					\
		sum -= lookup[k];			\
	} while( --j );					\
	last2 += incpitch2;				\
}

#define ADD_COLUMN()				\
{									\
	src0 -= incpitch0;				\
	int j = yr;						\
	do								\
	{								\
		int	k = *src0;				\
		sum += wlookup[k];			\
		table[k] += weight;			\
		src0 += spitch0;			\
	} while( --j );					\
									\
	src1 -= incpitch1;				\
	j = yr;							\
	do								\
	{								\
		int	k = *src1;				\
		sum += lookup[k];			\
		table[k]++;					\
		src1 += spitch1;			\
	} while( --j );					\
									\
	src2 -= incpitch2;				\
	j = yr;							\
	do								\
	{								\
		int	k = *src2;				\
		sum += lookup[k];			\
		table[k]++;					\
		src2 += spitch2;			\
	} while( --j );					\
}

#define SUBTRACT_COLUMN()			\
{									\
	int j = yr;						\
	do								\
	{								\
		int	k = *last0;				\
		sum -= wlookup[k];			\
		table[k] -= weight;			\
		last0 += spitch0;			\
	} while( --j );					\
	last0 -= (spitch0 - STEP);			\
									\
	j = yr;							\
	do								\
	{								\
		int	k = *last1;				\
		sum -= lookup[k];			\
		table[k]--;					\
		last1 += spitch1;			\
	} while( --j );					\
	last1 -= (spitch1 - 1);			\
									\
	j = yr;							\
	do								\
	{								\
		int	k = *last2;				\
		sum -= lookup[k];			\
		table[k]--;					\
		last2 += spitch2;			\
	} while( --j );					\
	last2 -= (spitch2 - 1);			\
}

#undef	RG2
#ifdef	PROFILE_VERSION
#define RG2(inc, label)								\
{													\
	int	val = (dst += inc)[0];						\
	if( sum < 0 )									\
	{												\
		do { ++profile; } while( (sum += table[++lval]) < 0 );	\
		if( val <= lval ) goto setval##label;		\
		if( (sum -= ulimit) > 0 )					\
			goto setvalrestsum##label;				\
		below##label:								\
		do											\
		{											\
			 ++profile;								\
			if( (sum += table[++lval]) > 0 )		\
				goto setvalrestsum##label;			\
		} while( lval != val );						\
		sum += ulimit;								\
		goto do_nothing##label;						\
		setvalrestsum##label:						\
		sum += ulimit;								\
		setval##label:								\
		dst[0] = lval;								\
		goto do_nothing##label;						\
	}												\
	if( (sum -= ulimit) > 0 )						\
	{ 												\
		do { ++profile; } while( (sum -= table[lval--]) > 0 );	\
		if( val > lval )							\
		{											\
			sum += ulimit;							\
			dst[0] = lval + 1;						\
			goto do_nothing##label;					\
		}											\
		if( (sum += ulimit) < 0 )					\
			goto setvaln##label;					\
		/* lval >= val */							\
		do											\
		{											\
			++profile;								\
			if( val == lval )						\
				goto do_nothing##label;				\
		} while( (sum -= table[lval--]) >= 0 );		\
		setvaln##label:								\
		dst[0] = lval + 1;							\
		goto do_nothing##label;						\
	}												\
	/* lval is now within the RG limits */			\
	/* sum -= ulimit */								\
	if( lval < val ) goto below##label;				\
	sum += ulimit;									\
	if( lval > val )								\
	{												\
		do											\
		{											\
			++profile;								\
			if( (sum -= table[lval--]) < 0 )		\
				goto setvaln##label;				\
		} while( lval != val );						\
	}												\
	/* lval == val */								\
	do_nothing##label:								\
	lookup = ltable + TABLE_MAX - lval;				\
	wlookup = wltable + TABLE_MAX - lval;			\
}

#else // PROFILE_VERSION
#define RG2(inc, label)								\
{													\
	int	val = (dst += inc)[0];						\
	if( sum < 0 )									\
	{												\
		do {} while( (sum += table[++lval]) < 0 );	\
		if( val <= lval ) goto setval##label;		\
		if( (sum -= ulimit) > 0 )					\
			goto setvalrestsum##label;				\
		below##label:								\
		do											\
		{											\
			if( (sum += table[++lval]) > 0 )		\
				goto setvalrestsum##label;			\
		} while( lval != val );						\
		sum += ulimit;								\
		goto do_nothing##label;						\
		setvalrestsum##label:						\
		sum += ulimit;								\
		setval##label:								\
		dst[0] = lval;								\
		goto do_nothing##label;						\
	}												\
	if( (sum -= ulimit) > 0 )						\
	{ 												\
		do {} while( (sum -= table[lval--]) > 0 );	\
		if( val > lval )							\
		{											\
			sum += ulimit;							\
			dst[0] = lval + 1;						\
			goto do_nothing##label;					\
		}											\
		if( (sum += ulimit) < 0 )					\
			goto setvaln##label;					\
		/* lval >= val */							\
		do											\
		{											\
			if( val == lval )						\
				goto do_nothing##label;				\
		} while( (sum -= table[lval--]) >= 0 );		\
		setvaln##label:								\
		dst[0] = lval + 1;							\
		goto do_nothing##label;						\
	}												\
	/* lval is now within the RG limits */			\
	/* sum -= ulimit */								\
	if( lval < val ) goto below##label;				\
	sum += ulimit;									\
	if( lval > val )								\
	{												\
		do											\
		{											\
			if( (sum -= table[lval--]) < 0 )		\
				goto setvaln##label;				\
		} while( lval != val );						\
	}												\
	/* lval == val */								\
	do_nothing##label:								\
	lookup = ltable + TABLE_MAX - lval;				\
	wlookup = wltable + TABLE_MAX - lval;			\
}
#endif	// PROFILE_VERSION

#ifdef	INIT_FUNC
#undef	INIT_FUNC
#endif
#define INIT_FUNC()								\
	int ulimit, llimit = 0;						\
	int	*wlookup = wltable + TABLE_MAX - lval

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif	
		tremovegrain(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, const BYTE *src2, int spitch2, int w, int h, int **llimits, int **ulimits, int xradius, int yradius, int weight, int *wltable)
{
#		include "tskeleton.h"
}

#ifdef	ADD_LINE
#undef  ADD_LINE
#undef  SUBTRACT_LINE
#undef  ADD_COLUMN
#undef  SUBTRACT_COLUMN
#endif
#ifdef	LOOKUP
#define	ADD_LINE()					\
{									\
	int j = xr;						\
	do								\
	{								\
		int	k = *src0++;			\
		sum0 += lookup0[k];			\
		table[k]++;					\
	} while( --j );					\
	src0 += incpitch0;				\
									\
	j = xr;							\
	do								\
	{								\
		int	k = *src1++;			\
		sum1 += lookup1[k];			\
		table[k + TABLE_SIZE]++;	\
	} while( --j );					\
	src1 += incpitch1;				\
}

#define SUBTRACT_LINE()				\
{									\
	int j = xr;						\
	do								\
	{								\
		int k = *last0++;			\
		table[k]--;					\
		sum0 -= lookup0[k];			\
	} while( --j );					\
	last0 += incpitch0;				\
									\
	j = xr;							\
	do								\
	{								\
		int k = *last1++;			\
		table[k + TABLE_SIZE]--;	\
		sum1 -= lookup1[k];			\
	} while( --j );					\
	last1 += incpitch1;				\
}

#define ADD_COLUMN()				\
{									\
	src0 -= incpitch0;				\
	int j = yr;						\
	do								\
	{								\
		int	k = *src0;				\
		sum0 += lookup0[k];			\
		table[k]++;					\
		src0 += spitch0;			\
	} while( --j );					\
									\
	src1 -= incpitch1;				\
	j = yr;							\
	do								\
	{								\
		int	k = *src1;				\
		sum1 += lookup1[k];			\
		table[k + TABLE_SIZE]++;	\
		src1 += spitch1;			\
	} while( --j );					\
}

#define SUBTRACT_COLUMN()			\
{									\
	int j = yr;						\
	do								\
	{								\
		int	k = *last0;				\
		sum0 -= lookup0[k];			\
		table[k]--;					\
		last0 += spitch0;			\
	} while( --j );					\
	last0 -= (spitch0 - STEP);		\
									\
	j = yr;							\
	do								\
	{								\
		int	k = *last1;				\
		sum1 -= lookup1[k];			\
		table[k + TABLE_SIZE]--;	\
		last1 += spitch1;			\
	} while( --j );					\
	last1 -= (spitch1 - STEP);		\
}
#else	// LOOKUP
#define	ADD_LINE()					\
{									\
	int j = xr;						\
	do								\
	{								\
		int	k = *src0++;			\
		if( k <= lval0 ) ++sum0;	\
		table[k]++;					\
	} while( --j );					\
	src0 += incpitch0;				\
									\
	j = xr;							\
	do								\
	{								\
		int	k = *src1++;			\
		if( k <= lval1 ) ++sum1;	\
		table[k + TABLE_SIZE]++;	\
	} while( --j );					\
	src1 += incpitch1;				\
}

#define SUBTRACT_LINE()				\
{									\
	int j = xr;						\
	do								\
	{								\
		int k = *last0++;			\
		table[k]--;					\
		if( k <= lval0 ) --sum0;	\
	} while( --j );					\
	last0 += incpitch0;				\
									\
	j = xr;							\
	do								\
	{								\
		int k = *last1++;			\
		table[k + TABLE_SIZE]--;	\
		if( k <= lval1 ) --sum1;	\
	} while( --j );					\
	last1 += incpitch1;				\
}

#define ADD_COLUMN()				\
{									\
	src0 -= incpitch0;				\
	int j = yr;						\
	do								\
	{								\
		int	k = *src0;				\
		if( k <= lval0 ) ++sum0;	\
		table[k]++;					\
		src0 += spitch0;			\
	} while( --j );					\

	src1 -= incpitch1;				\
	j = yr;							\
	do								\
	{								\
		int	k = *src1;				\
		if( k <= lval1 ) ++sum1;	\
		table[k + TABLE_SIZE]++;	\
		src1 += spitch1;			\
	} while( --j );					\
}

#define SUBTRACT_COLUMN()			\
{									\
	int j = yr;						\
	do								\
	{								\
		int	k = *last0;				\
		if( k <= lval0 ) --sum0;	\
		table[k]--;					\
		last0 += spitch0;			\
	} while( --j );					\
	last0 -= (spitch0 - STEP);		\
									\
	j = yr;							\
	do								\
	{								\
		int	k = *last1;				\
		if( k <= lval1 ) --sum1;	\
		table[k + TABLE_SIZE]--;	\
		last1 += spitch1;			\
	} while( --j );					\
	last1 -= (spitch1 - STEP);		\
}
#endif	// LOOKUP

#ifdef	check_table
#undef	check_table
#endif
#define check_table(pval)										\
{																\
	int	sum = 0;												\
	i = 0;														\
	while(i <= lval0)											\
	{															\
		if(table[i] < 0) debug_printf("negativ value 1\n");		\
		sum += table[i++];										\
	}															\
	if( sum != sum0 ) debug_printf("incorrect sum %u, %u\n", sum, sum0);	\
	while(i <= TABLE_MAX)										\
	{															\
		if( table[i] < 0 ) debug_printf("negativ value 2\n");	\
		sum += table[i++];										\
	}															\
	if(sum != xr * yr) debug_printf("incorrect table sum\n");	\
																\
	sum = 0;													\
	i = 0;														\
	while(i <= lval1)											\
	{															\
		if(table[i + TABLE_SIZE] < 0) debug_printf("negativ value 1\n");	\
		sum += table[i++ + TABLE_SIZE];							\
	}															\
	if( sum != sum1 ) debug_printf("incorrect sum %u, %u\n", sum, sum1);	\
	while(i <= TABLE_MAX)										\
	{															\
		if( table[i + TABLE_SIZE] < 0 ) debug_printf("negativ value 2\n");	\
		sum += table[i++ + TABLE_SIZE];							\
	}															\
	if(sum != xr * yr) debug_printf("incorrect table sum\n");	\
}

#ifdef	LOOKUP
#ifdef	PROFILE_VERSION
#define RR(dinc, sinc0, sinc1)							\
	{													\
		dst += dinc;									\
		_src0 += sinc0;									\
		_src1 += sinc1;									\
		int	i;											\
		if( (i = (int)_src0[0] - lval0) > 0 )			\
		{												\
			int olval = lval0;							\
			do											\
			{											\
				sum0 += table[++lval0];					\
			} while( --i );								\
			profile += (lval0 - olval);					\
		}												\
		else if( i < 0 )								\
		{												\
			int olval = lval0;							\
			do											\
			{											\
				sum0 -= table[lval0--];					\
			} while( ++i );								\
			profile += (olval - lval0);					\
		}												\
		lookup0 = ltable + TABLE_MAX - lval0;			\
		if( (i = _src1[0]) >= lval0 )					\
		{												\
			if( sum0 <= sum1 )							\
			{											\
				do { ++profile; } while( (sum1 -= table[lval1-- + TABLE_SIZE]) >= sum0 );	\
			}											\
			do {++profile; } while( (sum1 += table[++lval1 + TABLE_SIZE]) < sum0 );	\
			if( lval1 < i )								\
			{											\
				i = lval1;								\
				if( i < lval0 ) i = lval0;				\
			}											\
		}												\
		else											\
		{												\
			int sum = sum0 - table[lval0];				\
			if( sum < sum1 )							\
			{											\
				do { ++profile; } while( (sum1 -= table[lval1-- + TABLE_SIZE]) > sum );	\
			}											\
			do { ++profile; } while( (sum1 += table[++lval1 + TABLE_SIZE]) <= sum );	\
			if( lval1 > i )								\
			{											\
				i = lval1;								\
				if( i > lval0 ) i = lval0;				\
			}											\
		}												\
		dst[0] = (BYTE)i;								\
		lookup1 = ltable + TABLE_MAX - lval1;			\
	}
#else // PROFILE_VERSION
#define RR(dinc, sinc0, sinc1)							\
	{													\
		dst += dinc;									\
		_src0 += sinc0;									\
		_src1 += sinc1;									\
		int	i;											\
		if( (i = (int)_src0[0] - lval0) > 0 )			\
		{												\
			do											\
			{											\
				sum0 += table[++lval0];					\
			} while( --i );								\
		}												\
		else if( i < 0 )								\
		{												\
			do											\
			{											\
				sum0 -= table[lval0--];					\
			} while( ++i );								\
		}												\
		lookup0 = ltable + TABLE_MAX - lval0;			\
		if( (i = _src1[0]) >= lval0 )					\
		{												\
			if( sum0 <= sum1 )							\
			{											\
				do {} while( (sum1 -= table[lval1-- + TABLE_SIZE]) >= sum0 );	\
			}											\
			do {} while( (sum1 += table[++lval1 + TABLE_SIZE]) < sum0 );	\
			if( lval1 < i )								\
			{											\
				i = lval1;								\
				if( i < lval0 ) i = lval0;				\
			}											\
		}												\
		else											\
		{												\
			int sum = sum0 - table[lval0];				\
			if( sum < sum1 )							\
			{											\
				do {} while( (sum1 -= table[lval1-- + TABLE_SIZE]) > sum );	\
			}											\
			do {} while( (sum1 += table[++lval1 + TABLE_SIZE]) <= sum );	\
			if( lval1 > i )								\
			{											\
				i = lval1;								\
				if( i > lval0 ) i = lval0;				\
			}											\
		}												\
		dst[0] = (BYTE)i;								\
		lookup1 = ltable + TABLE_MAX - lval1;			\
	}
#endif	// PROFILE_VERSION
#else	// LOOKUP
#ifdef	PROFILE_VERSION
#define RR(dinc, sinc0, sinc1)							\
	{													\
		dst += dinc;									\
		_src0 += sinc0;									\
		_src1 += sinc1;									\
		int	i;											\
		if( (i = (int)_src0[0] - lval0) > 0 )			\
		{												\
			int olval = lval0;							\
			do											\
			{											\
				sum0 += table[++lval0];					\
			} while( --i );								\
			profile += (lval0 - olval);					\
		}												\
		else if( i < 0 )								\
		{												\
			int olval = lval0;							\
			do											\
			{											\
				sum0 -= table[lval0--];					\
			} while( ++i );								\
			profile += (olval - lval0);					\
		}												\
		if( (i = _src1[0]) >= lval0 )					\
		{												\
			if( sum0 <= sum1 )							\
			{											\
				do { ++profile; } while( (sum1 -= table[lval1-- + TABLE_SIZE]) >= sum0 );	\
			}											\
			do {++profile; } while( (sum1 += table[++lval1 + TABLE_SIZE]) < sum0 );	\
			if( lval1 < i )								\
			{											\
				i = lval1;								\
				if( i < lval0 ) i = lval0;				\
			}											\
		}												\
		else											\
		{												\
			int sum = sum0 - table[lval0];				\
			if( sum < sum1 )							\
			{											\
				do { ++profile; } while( (sum1 -= table[lval1-- + TABLE_SIZE]) > sum );	\
			}											\
			do { ++profile; } while( (sum1 += table[++lval1 + TABLE_SIZE]) <= sum );	\
			if( lval1 > i )								\
			{											\
				i = lval1;								\
				if( i > lval0 ) i = lval0;				\
			}											\
		}												\
		dst[0] = (BYTE)i;								\
	}
#else // PROFILE_VERSION
#define RR(dinc, sinc0, sinc1)							\
	{													\
		dst += dinc;									\
		_src0 += sinc0;									\
		_src1 += sinc1;									\
		int	i;											\
		if( (i = (int)_src0[0] - lval0) > 0 )			\
		{												\
			do											\
			{											\
				sum0 += table[++lval0];					\
			} while( --i );								\
		}												\
		else if( i < 0 )								\
		{												\
			do											\
			{											\
				sum0 -= table[lval0--];					\
			} while( ++i );								\
		}												\
		if( (i = _src1[0]) >= lval0 )					\
		{												\
			if( sum0 <= sum1 )							\
			{											\
				do {} while( (sum1 -= table[lval1-- + TABLE_SIZE]) >= sum0 );	\
			}											\
			do {} while( (sum1 += table[++lval1 + TABLE_SIZE]) < sum0 );	\
			if( lval1 < i )								\
			{											\
				i = lval1;								\
				if( i < lval0 ) i = lval0;				\
			}											\
		}												\
		else											\
		{												\
			int sum = sum0 - table[lval0];				\
			if( sum < sum1 )							\
			{											\
				do {} while( (sum1 -= table[lval1-- + TABLE_SIZE]) > sum );	\
			}											\
			do {} while( (sum1 += table[++lval1 + TABLE_SIZE]) <= sum );	\
			if( lval1 > i )								\
			{											\
				i = lval1;								\
				if( i > lval0 ) i = lval0;				\
			}											\
		}												\
		dst[0] = (BYTE)i;								\
	}
#endif	// PROFILE_VERSION
#endif	// LOOKUP

#ifdef	PROCESS_TABLES1
#undef	PROCESS_TABLES1
#undef	PROCESS_TABLES2
#endif
#define	PROCESS_TABLES1(dinc, sinc0, sinc1, label)			RR(dinc, sinc0, sinc1)
#define	PROCESS_TABLES2(dinc, sinc0, sinc1, x, y, label)	RR(dinc, sinc0, sinc1)

#ifdef	INIT_FUNC
#undef	INIT_FUNC
#endif
#define INIT_FUNC()				\
	const BYTE	*_src0 = src0;	\
	const BYTE	*_src1 = src1;	\

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif	
		rank_repair(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, int w, int h, int xradius, int yradius)
{
#		include "skeletonr.h"
}

#ifndef	SMARTMEDIAN_ODD	
#define sm_offset	0
#endif

#ifdef	check_table
#undef	check_table
#endif
#define check_table(out)										\
{																\
	int	sum2 = 0;												\
	i = 0;														\
	while(i <= TABLE_MAX)										\
	{															\
		if(table[TABLE_MAX + i] < 0) debug_printf("negativ value %i\n", out);	\
		sum2 += table[TABLE_MAX + i];							\
		++i;													\
	}															\
	i = xr * yr;												\
	if( sum2 != i ) debug_printf("incorrect sum %i, %i, %i\n", sum2, i, out);	\
}

#define check_remove(out)	\
	if( table[TABLE_MAX + *last] <= 0 ) debug_printf("%i, %i, %i\n", (last - debug_ptr)/debug_pitch, (last - debug_ptr)%debug_pitch, out);

#ifdef	ADD_LINE
#undef  ADD_LINE
#undef  SUBTRACT_LINE
#undef  ADD_COLUMN
#undef  SUBTRACT_COLUMN
#endif
#define	ADD_LINE()					\
{									\
	int j = xr;						\
	do								\
	{								\
		table[TABLE_MAX + *src++]++;\
	} while( --j );					\
	src += incpitch;				\
}

#define SUBTRACT_LINE()				\
{									\
	int j = xr;						\
	do								\
	{								\
		table[TABLE_MAX + *last++]--;\
	} while( --j );					\
	last += incpitch;				\
}

#define ADD_COLUMN()				\
{									\
	src -= incpitch;				\
	int j = yr;						\
	do								\
	{								\
		table[TABLE_MAX + *src]++;	\
		src += spitch;				\
	} while( --j );					\
}

#define SUBTRACT_COLUMN()			\
{									\
	int j = yr;						\
	do								\
	{								\
		table[TABLE_MAX + *last]--;	\
		last += spitch;				\
	} while( --j );					\
	last -= (spitch - STEP);			\
}

#ifdef	TABLE_SIZE
#undef	TABLE_SIZE
#endif
#define	TABLE_SIZE	(3*TABLE_MAX + 1)

#ifdef	PROFILE_VERSION
#define emedian1(dinc, sinc)													\
{																				\
	unsigned center;															\
	int sum, hsum;																\
	if( (sum = (hsum = table[TABLE_MAX + (center = (_src += sinc)[0])]) - limit) < 0 )	\
	{																			\
		int i = center, j = center;												\
		do																		\
		{																		\
			--i;																\
			hsum += table[TABLE_MAX + i];										\
			sum += table[TABLE_MAX + i];										\
		} while( (sum += table[TABLE_MAX + ++j]) < 0 );							\
		profile += (j - center);												\
																				\
		if( (hsum -= hlimit) < 0 )												\
			do {} while( (hsum += table[TABLE_MAX + ++center]) < 0 );			\
		else if( (sum -= (hsum + sm_offset - table[TABLE_MAX + center]) ) < 0 )	\
			do {} while( (sum += table[TABLE_MAX + --center]) < 0 );			\
	}																			\
	dst += dinc;																\
	dst[0] = center;															\
}
#else // PROFILE_VERSION
#define emedian1(dinc, sinc)													\
{																				\
	unsigned center;															\
	int sum, hsum;																\
	if( (sum = (hsum = table[TABLE_MAX + (center = (_src += sinc)[0])]) - limit) < 0 )	\
	{																			\
		int i = center, j = center;												\
		do																		\
		{																		\
			--i;																\
			hsum += table[TABLE_MAX + i];										\
			sum += table[TABLE_MAX + i];										\
		} while( (sum += table[TABLE_MAX + ++j]) < 0 );							\
																				\
		if( (hsum -= hlimit) < 0 )												\
			do {} while( (hsum += table[TABLE_MAX + ++center]) < 0 );			\
		else if( (sum -= (hsum + sm_offset - table[TABLE_MAX + center]) ) < 0 )	\
			do {} while( (sum += table[TABLE_MAX + --center]) < 0 );			\
	}																			\
	dst += dinc;																\
	dst[0] = center;															\
}
#endif // PROFILE_VERSION

#ifdef	SMARTMEDIAN_ODD	
#define emedian2(dinc, sinc, x, y)	\
	limit = limits[x][y];			\
	sm_offset = limit & 1;			\
	hlimit = limit / 2 + sm_offset;	\
	emedian1(dinc, sinc)
#else
#define emedian2(dinc, sinc, x, y)	\
	hlimit = limits[x][y] / 2;		\
	limit = 2* hlimit;				\
	emedian1(dinc, sinc)
#endif

#ifdef	PROCESS_TABLE1
#undef	PROCESS_TABLE1
#undef	PROCESS_TABLE2
#endif
#define	PROCESS_TABLE1(dinc, sinc, label)		emedian1(dinc, sinc)
#define	PROCESS_TABLE2(dinc, sinc, x, y, label)	emedian2(dinc, sinc, x, y)

#ifdef	INIT_FUNC
#undef	INIT_FUNC
#endif
#ifdef	SMARTMEDIAN_ODD	
#define INIT_FUNC()					\
	const BYTE	*_src = src;		\
	int limit, hlimit;				\
	int	sm_offset;
#else
#define INIT_FUNC()					\
	const BYTE	*_src = src;		\
	int limit, hlimit;
#endif

#ifdef	LOOKUP
#undef	LOOKUP
#endif

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif	
		smartmedian(BYTE *dst, int dpitch, const BYTE *src, int spitch, int w, int h, int **limits, int xradius, int yradius
#ifdef	LOCALTABLE
						)
#else						
						, int corner_boxsize, int *table)
#endif
{
#		include "skeleton.h"
}

#ifdef	check_table
#undef	check_table
#endif
#define check_table(out)										\
{																\
	int	sum2 = 0;												\
	i = 0;														\
	while(i <= TABLE_MAX)										\
	{															\
		if(table[TABLE_MAX + i] < 0) debug_printf("negativ value %i\n", out);	\
		sum2 += table[TABLE_MAX + i];							\
		++i;													\
	}															\
	i = xr * yr * (2 + weight);												\
	if( sum2 != i ) debug_printf("incorrect sum %i, %i, %i\n", sum2, i, out);	\
}

#ifdef	ADD_LINE
#undef  ADD_LINE
#undef  SUBTRACT_LINE
#undef  ADD_COLUMN
#undef  SUBTRACT_COLUMN
#endif
#define	ADD_LINE()					\
{									\
	int j = xr;						\
	do								\
	{								\
		table[TABLE_MAX + *src0++] += weight;\
	} while( --j );					\
	src0 += incpitch0;				\
									\
	j = xr;							\
	do								\
	{								\
		table[TABLE_MAX + *src1++]++;\
	} while( --j );					\
	src1 += incpitch1;				\
									\
	j = xr;							\
	do								\
	{								\
		table[TABLE_MAX + *src2++]++;\
	} while( --j );					\
	src2 += incpitch2;				\
}									

#define SUBTRACT_LINE()				\
{									\
	int j = xr;						\
	do								\
	{								\
		table[TABLE_MAX + *last0++] -= weight;\
	} while( --j );					\
	last0 += incpitch0;				\
									\
	j = xr;							\
	do								\
	{								\
		table[TABLE_MAX + *last1++]--;\
	} while( --j );					\
	last1 += incpitch1;				\
									\
	j = xr;							\
	do								\
	{								\
		table[TABLE_MAX + *last2++]--;\
	} while( --j );					\
	last2 += incpitch2;				\
}

#define ADD_COLUMN()				\
{									\
	src0 -= incpitch0;				\
	int j = yr;						\
	do								\
	{								\
		table[TABLE_MAX + *src0] += weight;\
		src0 += spitch0;			\
	} while( --j );					\
									\
	src1 -= incpitch1;				\
	j = yr;							\
	do								\
	{								\
		table[TABLE_MAX + *src1]++;	\
		src1 += spitch1;			\
	} while( --j );					\
									\
	src2 -= incpitch2;				\
	j = yr;							\
	do								\
	{								\
		table[TABLE_MAX + *src2]++;	\
		src2 += spitch2;			\
	} while( --j );					\
}

#define SUBTRACT_COLUMN()			\
{									\
	int j = yr;						\
	do								\
	{								\
		table[TABLE_MAX + *last0] -= weight;\
		last0 += spitch0;			\
	} while( --j );					\
	last0 -= (spitch0 - STEP);			\
									\
	j = yr;							\
	do								\
	{								\
		table[TABLE_MAX + *last1]--;\
		last1 += spitch1;			\
	} while( --j );					\
	last1 -= (spitch1 - STEP);			\
									\
	j = yr;							\
	do								\
	{								\
		table[TABLE_MAX + *last2]--;\
		last2 += spitch2;			\
	} while( --j );					\
	last2 -= (spitch2 - STEP);			\
}

#ifdef	INIT_FUNC
#undef	INIT_FUNC
#endif
#ifdef	SMARTMEDIAN_ODD	
#define INIT_FUNC()					\
	const BYTE	*_src = src0;		\
	int limit, hlimit;				\
	int	sm_offset;
#else
#define INIT_FUNC()					\
	const BYTE	*_src = src0;		\
	int limit, hlimit;
#endif

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif	
		tsmart_median(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, const BYTE *src2, int spitch2, int w, int h, int **limits, int xradius, int yradius, int weight)
{
#		include "tskeleton.h"
}

#ifdef	check_table
#undef	check_table
#endif
#define check_table(out)										\
{																\
	int	sum2 = 0;												\
	i = 0;														\
	while(i <= TABLE_MAX)										\
	{															\
		if(table[TABLE_MAX + i] < 0) debug_printf("negativ value %i\n", out);	\
		sum2 += table[TABLE_MAX + i];							\
		++i;													\
	}															\
	i = xr * yr * (1 + weight);												\
	if( sum2 != i ) debug_printf("incorrect sum %i, %i, %i\n", sum2, i, out);	\
}

#ifdef	ADD_LINE
#undef  ADD_LINE
#undef  SUBTRACT_LINE
#undef  ADD_COLUMN
#undef  SUBTRACT_COLUMN
#endif
#define	ADD_LINE()					\
{									\
	int j = xr;						\
	do								\
	{								\
		table[TABLE_MAX + *src0++] += weight;\
	} while( --j );					\
	src0 += incpitch0;				\
									\
	j = xr;							\
	do								\
	{								\
		table[TABLE_MAX + *src1++]++;\
	} while( --j );					\
	src1 += incpitch1;				\
}									

#define SUBTRACT_LINE()				\
{									\
	int j = xr;						\
	do								\
	{								\
		table[TABLE_MAX + *last0++] -= weight;\
	} while( --j );					\
	last0 += incpitch0;				\
									\
	j = xr;							\
	do								\
	{								\
		table[TABLE_MAX + *last1++]--;\
	} while( --j );					\
	last1 += incpitch1;				\
}

#define ADD_COLUMN()				\
{									\
	src0 -= incpitch0;				\
	int j = yr;						\
	do								\
	{								\
		table[TABLE_MAX + *src0] += weight;\
		src0 += spitch0;			\
	} while( --j );					\
									\
	src1 -= incpitch1;				\
	j = yr;							\
	do								\
	{								\
		table[TABLE_MAX + *src1]++;	\
		src1 += spitch1;			\
	} while( --j );					\
}

#define SUBTRACT_COLUMN()			\
{									\
	int j = yr;						\
	do								\
	{								\
		table[TABLE_MAX + *last0] -= weight;\
		last0 += spitch0;			\
	} while( --j );					\
	last0 -= (spitch0 - STEP);			\
									\
	j = yr;							\
	do								\
	{								\
		table[TABLE_MAX + *last1]--;\
		last1 += spitch1;			\
	} while( --j );					\
	last1 -= (spitch1 - STEP);		\
}

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif	
		smart_median2(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, int w, int h, int **limits, int xradius, int yradius, int weight)
{
#		include "skeleton2.h"
}

static	IScriptEnvironment	*AVSenvironment;

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif		
		copy_plane(BYTE *dst, int dpitch, const BYTE *src, int spitch, int w, int h, int **limits, int xradius, int yradius
#ifdef	LOCALTABLE
				   )
#else
				   , int corner_boxsize, int *table)
#endif
{
	AVSenvironment->BitBlt(dst, dpitch, src, spitch, w, h);
#ifdef	PROFILE_VERSION
	return 0;
#endif
};

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif		
		do_nothing(BYTE *dst, int dpitch, const BYTE *src, int spitch, int w, int h, int **limits, int xradius, int yradius
#ifdef	LOCALTABLE
				   )
#else
				   , int corner_boxsize, int *table)
#endif
{
#ifdef	PROFILE_VERSION
	return 0;
#endif
};

typedef 
#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif	
		(*sqf)(BYTE *dst, int dpitch, const BYTE *src, int spitch, int w, int h, int **limits, int xradius, int yradius
#ifdef	LOCALTABLE
				   );
#else
				   , int corner_boxsize, int *table);
#endif

class	SingleQuantile : public GenericVideoFilter, public PlanarAccess
{
	int	**limits[3];
	sqf primary;
	sqf pfunction[3];
#ifndef	LOCALTABLE
	int corner_boxsize[3];
#endif
	int	xradius[3], yradius[3];
	int	inner_width[3], inner_height[3];
#ifdef	PROFILE_VERSION
	char		*fname;
	unsigned	size[3];
#endif
#ifndef	LOCALTABLE
	int	table[TABLE_MAX + 1];
#endif

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
#ifdef	PROFILE_VERSION
		unsigned	profile[3];
#endif
		PVideoFrame df = env->NewVideoFrame(vi);
		PVideoFrame sf = child->GetFrame(n, env);

		int	i = planes;
		do
		{
#ifdef	PROFILE_VERSION
			profile[i] =
#endif
			pfunction[i](GetWritePtr(df, i), GetPitch(df, i), GetReadPtr(sf, i), GetPitch(sf, i), inner_width[i], inner_height[i], limits[i], xradius[i], yradius[i]
#ifdef	LOCALTABLE
						   );
#else			
							, corner_boxsize[i], table);
#endif
		} while( --i >= 0 );
#ifdef	PROFILE_VERSION
		debug_printf("[%i] %s Profile: %f, %f, %f\n", n, fname, ((double) profile[0]) / size[0], ((double) profile[1]) / size[1], ((double) profile[2]) / size[2]);
#endif
		return df;
	};
public:
	SingleQuantile(PClip clip, int *_xradius, int *_yradius, int *_limit, bool planar, sqf func, const char *name) : primary(func), GenericVideoFilter(clip), PlanarAccess(vi)
	{
		if(planes == 0) AVSenvironment->ThrowError("%s: only planar color spaces are allowed", name);
		
#ifndef	LOCALTABLE		
		memset(table, 0, sizeof(int)*TABLE_SIZE);
#endif
#ifdef	PROFILE_VERSION
		fname = name;
#endif
		
		int i = planes;
		do
		{
#ifdef	PROFILE_VERSION
			size[i] = width[i] * height[i];
#endif
			xradius[i] = _xradius[i];
			yradius[i] = _yradius[i];
			if( (xradius[i] < 0) || (yradius[i] < 0) )
			{
				pfunction[i] = do_nothing;
			}
			else
			{
				inner_width[i] = width[i]; inner_height[i] = height[i];
				if( (xradius[i] == 0) || (yradius[i] == 0) )
				{
					pfunction[i] = copy_plane;
				}
				else
				{
					pfunction[i] = primary;
					int	xr, yr;
					if( ((inner_width[i] -= (xr = 2 * xradius[i] + 1)) <= 0) || ((inner_height[i] -= (yr = 2 * yradius[i] + 1)) <= 0) )
					AVSenvironment->ThrowError("%s: width or height of the clip is to small!", name);
					int	x = xradius[i] + 1;
					limits[i] = (new int *[x]) - x;
					int ya = yradius[i] + 1;
					int	boxsize = xr * yr;
					int boxsize2 = (boxsize + 1)/ 2;
#ifdef	LOCALTABLE
					int	*mem = new int [x * ya] - ya;
#else
					int	*mem = new int [corner_boxsize[i] = x * ya] - ya;
#endif
					do
					{
						limits[i][x] = mem;
						mem += ya;
						int y = ya;
						do
						{
							int	k = x * y;
							if( _limit[i] == boxsize2 ) k = (k + 1) / 2;
							else if( (k = ((k * _limit[i]) + boxsize2) / boxsize) == 0 ) ++k;
							limits[i][x][y] = k;
						} while( ++y <= yr );
					} while( ++x <= xr );
				}
			}
		} while( --i >= 0 );
	}
	
	~SingleQuantile()
	{
		int i = planes;
		do
		{
			if( pfunction[i] == primary )
			{
				delete [] (limits[i][xradius[i] + 1] + (yradius[i] + 1));
				delete [] (limits[i] + xradius[i] + 1);
			}
		} while( --i >= 0 );
	}
};

int	QuantileDefault(int xradius, int yradius)
{
	return ((2*xradius + 1)*(2*yradius + 1) + 1)/2;
}

int	SmartMedianDefault(int xradius, int yradius)
{
	return 4 * min(xradius, yradius) + 2;
}

AVSValue __cdecl CreateSingleQuantile(AVSValue args, sqf func, int dfunc(int, int), const char *name)
{
	enum ARGS { CLIP, RADIUS, LIMIT, RADIUS_Y, YRADIUS_Y, LIMIT_Y, RADIUS_U, YRADIUS_U, LIMIT_U
					, XRADIUS_V, YRADIUS_V, LIMIT_V, PLANAR};

	int	xradius[3], yradius[3], limits[3];
	int radius = args[RADIUS].AsInt(DEFAULT_RADIUS);
	int limit = args[LIMIT].AsInt(dfunc(radius, radius));
	
	int i = 2, j = 6;
	do
	{
		xradius[i] = args[RADIUS_Y + j].AsInt(radius);
		yradius[i] = args[YRADIUS_Y + j].AsInt(xradius[i]);
		limits[i] = args[LIMIT_Y + j].AsInt(args[RADIUS_Y + j].Defined() || args[RADIUS_Y + j].Defined() ? dfunc(xradius[i], yradius[i]) : limit);
		int boxsize = (2 * xradius[i] + 1) * (2 * yradius[i] + 1);
		if( limits[i] > boxsize ) limits[i] = boxsize;
		if( limits[i] <= 0 ) limits[i] = 1;
		j -= 3;
	} while( --i >= 0 );
	
	return new SingleQuantile(args[CLIP].AsClip(), xradius, yradius, limits, args[PLANAR].AsBool(false), func, name);
}

AVSValue __cdecl CreateQuantile(AVSValue args, void* user_data, IScriptEnvironment* env)
{	
	return CreateSingleQuantile(args, single_quantile, QuantileDefault, "Quantile");
}

AVSValue __cdecl CreateSmartMedian(AVSValue args, void* user_data, IScriptEnvironment* env)
{	
	return CreateSingleQuantile(args, smartmedian, SmartMedianDefault, "SmartMedian");
}

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif
	rg_do_nothing(BYTE *dst, int dpitch, const BYTE *src, int spitch, int w, int h, int **llimits, int **ulimits, int xradius, int yradius)
{
#ifdef	PROFILE_VERSION
	return	0;
#endif
}

#define	COMPARE_MASK	(~24)
	
static	void CompareVideoInfo(VideoInfo &vi1, const VideoInfo &vi2, const char *progname)
{	
	if( (vi1.width != vi2.width) || (vi1.height != vi2.height) || ( (vi1.pixel_type & COMPARE_MASK) != (vi2.pixel_type & COMPARE_MASK) ))
	{
#if	0
		debug_printf("widths = %u, %u, heights = %u, %u, color spaces = %X, %X\n"
						, vi1.width, vi2.width, vi1.height, vi2.height, vi1.pixel_type, vi2.pixel_type);
#endif
		AVSenvironment->ThrowError("%s: clips must be of equal type", progname);
	}
	if(vi1.num_frames > vi2.num_frames) vi1.num_frames = vi2.num_frames;
}

typedef 
#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif
	(*rgf)(BYTE *dst, int dpitch, const BYTE *src, int spitch, int w, int h, int **llimits, int **ulimits, int xradius, int yradius);

class	RemoveGrainHD : public GenericVideoFilter, public PlanarAccess
{
	PClip child2;
	int	**llimits[3];
	int	**ulimits[3];
	rgf pfunction[3];
	int	xradius[3], yradius[3];
	int	inner_width[3], inner_height[3];
#ifdef	PROFILE_VERSION
	char		*fname;
	unsigned	size[3];
#endif

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
#ifdef	AVS_COPY
		PVideoFrame df = child2->GetFrame(n, env);
		if( AVSenvironment->MakeWritable(&df) == false ) AVSenvironment->ThrowError("RemoveGrainHD: MakeWritable failed");
#else
		PVideoFrame df0 = child2->GetFrame(n, env);
		PVideoFrame df = AVSenvironment->NewVideoFrame(vi);
#endif
		PVideoFrame sf = child->GetFrame(n, env);
#ifdef	PROFILE_VERSION
		unsigned	profile[3];
#endif
		
		int	i = planes;
		do
		{
#ifndef	AVS_COPY
			AVSenvironment->BitBlt(GetWritePtr(df, i), GetPitch(df, i), GetReadPtr(df0, i), GetPitch(df0, i), width[i], height[i]);
#endif
#ifdef	PROFILE_VERSION
			profile[i] =
#endif
			pfunction[i](GetWritePtr(df, i), GetPitch(df, i), GetReadPtr(sf, i), GetPitch(sf, i), inner_width[i], inner_height[i], llimits[i], ulimits[i], xradius[i], yradius[i]);
		} while( --i >= 0 );
#ifdef	PROFILE_VERSION
		debug_printf("[%i] RemoveGrainHD Profile: %f, %f, %f\n", n, ((double) profile[0]) / size[0], ((double) profile[1]) / size[1], ((double) profile[2]) / size[2]);
#endif
		return df;
	};
public:
	RemoveGrainHD(PClip clip, PClip _child2, int *_xradius, int *_yradius, int *_llimit, int *_ulimit, bool planar) : GenericVideoFilter(clip), PlanarAccess(vi), child2(_child2)
	{
		if(planes == 0) AVSenvironment->ThrowError("RemoveGrainHD: only planar color spaces are allowed");
	
		CompareVideoInfo(vi, child2->GetVideoInfo(), "RemoveGrainHD"); 

		int i = planes;
		do
		{
#ifdef	PROFILE_VERSION
			size[i] = width[i] * height[i];
#endif
			xradius[i] = _xradius[i];
			yradius[i] = _yradius[i];
			if( (xradius[i] <= 0) || (yradius[i] <= 0) )
			{
				pfunction[i] = rg_do_nothing;
			}
			else
			{
				pfunction[i] = removegrain;
				int	xr, yr;
				if( ((inner_width[i] = width[i] - (xr = 2 * xradius[i] + 1)) <= 0) || ((inner_height[i] = height[i] - (yr = 2 * yradius[i] + 1)) <= 0) )
					AVSenvironment->ThrowError("RemoveGrainHD: width or height of the clip is to small!");
				int	x = xradius[i] + 1;
				int	boxsize = xr * yr;
				int boxsize2 = (boxsize + 1)/ 2;
				int ya = yradius[i] + 1;
				llimits[i] = (ulimits[i] = new int *[2*x]) - x;
				int	*mem = new int [2 * x * ya];
				do
				{
					llimits[i][x] = (ulimits[i][x] = mem) - ya;
					mem += 2*ya;
					int y = ya;
					do
					{
						int	k;
						int	cboxsize = x * y;
						if( (k = (cboxsize * _llimit[i] + boxsize2) / boxsize) == 0 ) ++k;
						llimits[i][x][y] = k;
						int m;
						if( (m = (cboxsize * _ulimit[i] + boxsize2) / boxsize) == 0 ) ++m;
						if( (ulimits[i][x][y] = cboxsize - m - k) < 0 ) ulimits[i][x][y] = -1;
					} while( ++y <= yr );
				} while( ++x <= xr );
			}
		} while( --i >= 0 );
	}

	~RemoveGrainHD()
	{
		int i = planes;
		do
		{
			if( pfunction[i] == removegrain )
			{
				delete [] ulimits[i][xradius[i] + 1];
				delete [] ulimits[i];
			}
		} while( --i >= 0 );
	}
};

AVSValue __cdecl CreateRemoveGrainHD(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	enum ARGS { CLIP, CHILD2, RADIUS, RANK, RADIUS_Y, YRADIUS_Y, RANK_Y, URANK_Y, RADIUS_U, YRADIUS_U, RANK_U, URANK_U
					, XRADIUS_V, YRADIUS_V, RANK_V, URANK_V, PLANAR};

	int	xradius[3], yradius[3], llimit[3], ulimit[3];
	int radius = args[RADIUS].AsInt(DEFAULT_RADIUS);
	int limit = args[RANK].AsInt(2*radius + 1);
	
	int i = 2, j = 8; 
	do
	{
		xradius[i] = args[RADIUS_Y + j].AsInt(radius);
		yradius[i] = args[YRADIUS_Y + j].AsInt(xradius[i]);
		llimit[i] = args[RANK_Y + j].AsInt(args[RADIUS_Y + j].Defined() || args[RADIUS_Y + j].Defined() ? 2 * min(xradius[i], yradius[i]) + 1 : limit);
		int maxsize = (2 * xradius[i] + 1) * (2 * yradius[i] + 1);
		if( llimit[i] > maxsize ) llimit[i] = maxsize;
		ulimit[i] = args[URANK_Y + j].AsInt(llimit[i]);
		if( ulimit[i] <= 0 ) ulimit[i] = 1;
		j -= 4;
	} while( --i >= 0 );
	
	PClip clip = args[CLIP].AsClip();
	return new RemoveGrainHD(clip, args[CHILD2].Defined() ? args[CHILD2].AsClip() : clip, xradius, yradius, llimit, ulimit, args[PLANAR].AsBool(false));
}


#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif			
		trg_do_nothing(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, const BYTE *src2, int spitch2, int w, int h, int **llimits, int **ulimits, int xradius, int yradius, int weight, int *wltable)
{
#ifdef	PROFILE_VERSION
	return	0;
#endif
}

typedef 
#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif
	(*trgf)(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, const BYTE *src2, int spitch2, int w, int h, int **llimits, int **ulimits, int xradius, int yradius, int weight, int *wltable);

class	TemporalRemoveGrainHD : public GenericVideoFilter, public PlanarAccess
{
	PClip child2;
	int	wltable[2*TABLE_MAX + 1];
	int	**llimits[3];
	int	**ulimits[3];
	trgf pfunction[3];
	int	xradius[3], yradius[3];
	int	inner_width[3], inner_height[3];
	int	weight;
#ifdef	PROFILE_VERSION
	unsigned	size[3];
#endif
	unsigned	lastframe;
	
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
#ifdef	AVS_COPY
		PVideoFrame df = child2->GetFrame(n, env);
		if( (unsigned) (n - 1) > lastframe ) return df;
#else
		PVideoFrame df0 = child2->GetFrame(n, env);
		if( (unsigned) (n - 1) > lastframe ) return df0;
#endif
		{
#ifdef	AVS_COPY
			if( AVSenvironment->MakeWritable(&df) == false ) AVSenvironment->ThrowError("TemporalRemoveGrainHD: MakeWritable failed");
#else
			PVideoFrame df = AVSenvironment->NewVideoFrame(vi);
#endif
			PVideoFrame sf1 = child->GetFrame(n - 1, env);
			PVideoFrame sf0 = child->GetFrame(n, env);
			PVideoFrame sf2 = child->GetFrame(n + 1, env);

#ifdef	PROFILE_VERSION
			unsigned	profile[3];
#endif
		
			int	i = planes;
			do
			{
#ifndef	AVS_COPY
				AVSenvironment->BitBlt(GetWritePtr(df, i), GetPitch(df, i), GetReadPtr(df0, i), GetPitch(df0, i), width[i], height[i]);
#endif
#ifdef	PROFILE_VERSION
				profile[i] =
#endif
				pfunction[i](GetWritePtr(df, i), GetPitch(df, i), GetReadPtr(sf0, i), GetPitch(sf0, i), GetReadPtr(sf1, i), GetPitch(sf1, i), GetReadPtr(sf2, i), GetPitch(sf2, i), inner_width[i], inner_height[i], llimits[i], ulimits[i], xradius[i], yradius[i], weight, wltable);
			} while( --i >= 0 );
#ifdef	PROFILE_VERSION
			debug_printf("[%i] TemporalRemoveGrainHD Profile: %f, %f, %f\n", n, ((double) profile[0]) / size[0], ((double) profile[1]) / size[1], ((double) profile[2]) / size[2]);
#endif
			return df;
		}
	};
public:
	TemporalRemoveGrainHD(PClip clip, PClip _child2, int *_xradius, int *_yradius, int *_llimit, int *_ulimit, int _weight, bool planar) : GenericVideoFilter(clip), PlanarAccess(vi), child2(_child2), weight(_weight)
	{
		if(planes == 0) AVSenvironment->ThrowError("TemporalRemoveGrainHD: only planar color spaces are allowed");
	
		CompareVideoInfo(vi, child2->GetVideoInfo(), "TemporalRemoveGrainHD"); 

		if( (int)(lastframe = vi.num_frames - 3) < 0 ) AVSenvironment->ThrowError("TemporalRemoveGrainHD: input clip too short");

		/* initialise wltable */
		int	i = TABLE_MAX;
		do
		{
			wltable[TABLE_MAX + i] = 0;
		} while ( --i );
		i = TABLE_MAX;
		do
		{
			wltable[i] = weight;
		} while ( --i >= 0 );

		int	factor = 2 + weight;

		i = planes;
		do
		{
#ifdef	PROFILE_VERSION
			size[i] = width[i] * height[i];
#endif
			xradius[i] = _xradius[i];
			yradius[i] = _yradius[i];
			if( (xradius[i] <= 0) || (yradius[i] <= 0) )
			{
				pfunction[i] = trg_do_nothing;
			}
			else
			{
				pfunction[i] = tremovegrain;
				int	xr, yr;
				if( ((inner_width[i] = width[i] - (xr = 2 * xradius[i] + 1)) <= 0) || ((inner_height[i] = height[i] - (yr = 2 * yradius[i] + 1)) <= 0) )
					AVSenvironment->ThrowError("TemporalRemoveGrainHD: width or height of the clip is to small!");
				int	x = xradius[i] + 1;
				int	boxsize = xr * yr;
				int boxsize2 = (boxsize + 1)/ 2;
				int ya = yradius[i] + 1;
				llimits[i] = (ulimits[i] = new int *[2*x]) - x;
				int	*mem = new int [2 * x * ya];
				do
				{
					llimits[i][x] = (ulimits[i][x] = mem) - ya;
					mem += 2*ya;
					int y = ya;
					do
					{
						int	k;
						int	cboxsize = x * y;
						if( (k = (cboxsize * _llimit[i] + boxsize2) / boxsize) <= 0 ) k = 1;
						llimits[i][x][y] = k;
						int m;
						if( (m = (cboxsize * _ulimit[i] + boxsize2) / boxsize) == 0 ) ++m;
						if( (ulimits[i][x][y] = factor * cboxsize - m - k) < 0 ) ulimits[i][x][y] = -1;
					} while( ++y <= yr );
				} while( ++x <= xr );
			}
		} while( --i >= 0 );
	}

	~TemporalRemoveGrainHD()
	{
		int i = planes;
		do
		{
			if( pfunction[i] != trg_do_nothing )
			{
				delete [] ulimits[i][xradius[i] + 1];
				delete [] ulimits[i];
			}
		} while( --i >= 0 );
	}
};


AVSValue __cdecl CreateTemporalRemoveGrainHD(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	enum ARGS { CLIP, CHILD2, RADIUS, RANK, RADIUS_Y, YRADIUS_Y, RANK_Y, URANK_Y, RADIUS_U, YRADIUS_U, RANK_U, URANK_U
					, XRADIUS_V, YRADIUS_V, RANK_V, URANK_V, WEIGHT, PLANAR};

	int	xradius[3], yradius[3], llimit[3], ulimit[3];
	int radius = args[RADIUS].AsInt(DEFAULT_RADIUS);
	int weight = args[WEIGHT].AsInt(1);
	int	factor = 2 + weight;
	int limit = args[RANK].AsInt(factor*(2*radius + 1));
	
	int i = 2, j = 8; 
	do
	{
		xradius[i] = args[RADIUS_Y + j].AsInt(radius);
		yradius[i] = args[YRADIUS_Y + j].AsInt(xradius[i]);
		llimit[i] = args[RANK_Y + j].AsInt(args[RADIUS_Y + j].Defined() || args[RADIUS_Y + j].Defined() ? factor * (2 * min(xradius[i], yradius[i]) + 1) : limit);
		int maxsize = factor * (2 * xradius[i] + 1) * (2 * yradius[i] + 1);
		if( llimit[i] > maxsize ) llimit[i] = maxsize;
		ulimit[i] = args[URANK_Y + j].AsInt(llimit[i]);
		if( ulimit[i] <= 0 ) ulimit[i] = 1;
		j -= 4;
	} while( --i >= 0 );
	
	PClip clip = args[CLIP].AsClip();
	return new TemporalRemoveGrainHD(clip, args[CHILD2].Defined() ? args[CHILD2].AsClip() : clip, xradius, yradius, llimit, ulimit, weight, args[PLANAR].AsBool(false));
}

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif		
		tsm_do_nothing(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, const BYTE *src2, int spitch2, int w, int h, int **limits, int xradius, int yradius, int weight)
{
#ifdef	PROFILE_VERSION
	return 0;
#endif	
}

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif	
		tsm_copy_plane(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, const BYTE *src2, int spitch2, int w, int h, int **limits, int xradius, int yradius, int weight)
{
	AVSenvironment->BitBlt(dst, dpitch, src0, spitch0, w, h);
#ifdef	PROFILE_VERSION
	return 0;
#endif
};

class	TemporalSmartMedian : public GenericVideoFilter, public PlanarAccess
{
	int	**limits[3];
#ifdef	PROFILE_VERSION
	unsigned	size[3];
unsigned
#else
void
#endif	
			(*pfunction[3])(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, const BYTE *src2, int spitch2, int w, int h, int **limits, int xradius, int yradius, int weight);
	int	xradius[3], yradius[3];
	int	inner_width[3], inner_height[3];
	int weight;
	unsigned	lastframe;
	
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
		if( (unsigned) (n - 1) > lastframe ) return child->GetFrame(n, env);
#ifdef	PROFILE_VERSION
		unsigned	profile[3];
#endif
		PVideoFrame df = env->NewVideoFrame(vi);
		PVideoFrame sf1 = child->GetFrame(n - 1, env);
		PVideoFrame sf0 = child->GetFrame(n, env);
		PVideoFrame sf2 = child->GetFrame(n + 1, env);
		
		int	i = planes;
		do
		{
#ifdef	PROFILE_VERSION
			profile[i] =
#endif
			pfunction[i](GetWritePtr(df, i), GetPitch(df, i), GetReadPtr(sf0, i), GetPitch(sf0, i), GetReadPtr(sf1, i), GetPitch(sf1, i), GetReadPtr(sf2, i), GetPitch(sf2, i), inner_width[i], inner_height[i], limits[i], xradius[i], yradius[i], weight);
		} while( --i >= 0 );
#ifdef	PROFILE_VERSION
		debug_printf("[%i] TemporalSmartMedian Profile: %f, %f, %f\n", n, ((double) profile[0]) / size[0], ((double) profile[1]) / size[1], ((double) profile[2]) / size[2]);
#endif
		return df;	
	};
public:
	TemporalSmartMedian(PClip clip, int *_xradius, int *_yradius, int *_limit, int _weight, bool planar) : GenericVideoFilter(clip), PlanarAccess(vi), weight(_weight)
	{
		if(planes == 0) AVSenvironment->ThrowError("TemporalSmartMedian: only planar color spaces are allowed");
	
		if( (int)(lastframe = vi.num_frames - 3) < 0 ) AVSenvironment->ThrowError("TemporalSmartMedian: input clip too short");

		int i = planes;
		do
		{
#ifdef	PROFILE_VERSION
			size[i] = width[i] * height[i];
#endif
			xradius[i] = _xradius[i];
			yradius[i] = _yradius[i];
			if( (xradius[i] < 0) || (yradius[i] < 0) )
			{
				pfunction[i] = tsm_do_nothing;
			}
			else
			{
				inner_width[i] = width[i]; inner_height[i] = height[i];
				if( (xradius[i] == 0) || (yradius[i] == 0) )
				{
					pfunction[i] = tsm_copy_plane;
				}
				else
				{
					pfunction[i] = tsmart_median;
					int	xr, yr;
					if( ((inner_width[i] -= (xr = 2 * xradius[i] + 1)) <= 0) || ((inner_height[i] -= (yr = 2 * yradius[i] + 1)) <= 0) )
						AVSenvironment->ThrowError("TemporalSmartMedian: width or height of the clip is to small!");
					int	x = xradius[i] + 1;
					int	boxsize = xr * yr;
					int boxsize2 = (boxsize + 1)/ 2;
					int ya = yradius[i] + 1;
					limits[i] = new int *[x] - x;
					int	*mem = new int [x * ya] - ya;
					do
					{
						limits[i][x] = mem;
						mem += ya;
						int y = ya;
						do
						{
							int	k = x * y;
							if( (k = ((k * _limit[i]) + boxsize2) / boxsize) == 0 ) ++k;
							limits[i][x][y] = k;
						} while( ++y <= yr );
					} while( ++x <= xr );
				}
			}
		} while( --i >= 0 );
	}

	~TemporalSmartMedian()
	{
		int i = planes;
		do
		{
			if( pfunction[i] == tsmart_median )
			{
				delete [] (limits[i][xradius[i] + 1] + (yradius[i] + 1));
				delete [] (limits[i] + xradius[i] + 1);
			}
		} while( --i >= 0 );
	}
};

AVSValue __cdecl CreateTemporalSmartMedian(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	enum ARGS { CLIP, RADIUS, PIXELS, RADIUS_Y, YRADIUS_Y, PIXELS_Y, RADIUS_U, YRADIUS_U, PIXELS_U
					, RADIUS_V, YRADIUS_V, PIXELS_V, WEIGHT, PLANAR};

	int	xradius[3], yradius[3], limits[3];
	int radius = args[RADIUS].AsInt(DEFAULT_RADIUS);
	int weight = args[WEIGHT].AsInt(1);
	if( weight < 0 ) weight = 0;
	if( weight > 100 ) weight = 100;
	int	factor = 2 + weight;
	int limit = args[PIXELS].AsInt(factor*(4*radius + 2));
	
	int i = 2, j = 6; 
	do
	{
		xradius[i] = args[RADIUS_Y + j].AsInt(radius);
		yradius[i] = args[YRADIUS_Y + j].AsInt(xradius[i]);
		limits[i] = args[PIXELS + j].AsInt(args[RADIUS_Y + j].Defined() || args[RADIUS_Y + j].Defined() ? factor * (4 * min(xradius[i], yradius[i]) + 2) : limit);
		int maxsize = factor * (2 * xradius[i] + 1) * (2 * yradius[i] + 1);
		if( limits[i] > maxsize ) limits[i] = maxsize;
		if( limits[i] <= 0 ) limits[i] = 1;
		j -= 3;
	} while( --i >= 0 );
	
	return new TemporalSmartMedian(args[CLIP].AsClip(), xradius, yradius, limits, weight, args[PLANAR].AsBool(false));
}

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif		
		sm2_do_nothing(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, int w, int h, int **limits, int xradius, int yradius, int weight)
{
#ifdef	PROFILE_VERSION
	return 0;
#endif	
}

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif	
		sm2_copy_plane(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, int w, int h, int **limits, int xradius, int yradius, int weight)
{
	AVSenvironment->BitBlt(dst, dpitch, src0, spitch0, w, h);
#ifdef	PROFILE_VERSION
	return 0;
#endif
};

class	SmartMedian2 : public GenericVideoFilter, public PlanarAccess
{
	PClip	child2;
	int	**limits[3];
#ifdef	PROFILE_VERSION
	unsigned	size[3];
unsigned
#else
void
#endif	
			(*pfunction[3])(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, int w, int h, int **limits, int xradius, int yradius, int weight);
	int	xradius[3], yradius[3];
	int	inner_width[3], inner_height[3];
	int weight;
	
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
#ifdef	PROFILE_VERSION
		unsigned	profile[3];
#endif
		PVideoFrame df = env->NewVideoFrame(vi);
		PVideoFrame sf1 = child2->GetFrame(n, env);
		PVideoFrame sf0 = child->GetFrame(n, env);
		
		int	i = planes;
		do
		{
#ifdef	PROFILE_VERSION
			profile[i] =
#endif
			pfunction[i](GetWritePtr(df, i), GetPitch(df, i), GetReadPtr(sf0, i), GetPitch(sf0, i), GetReadPtr(sf1, i), GetPitch(sf1, i), inner_width[i], inner_height[i], limits[i], xradius[i], yradius[i], weight);
		} while( --i >= 0 );
#ifdef	PROFILE_VERSION
		debug_printf("[%i] TemporalSmartMedian Profile: %f, %f, %f\n", n, ((double) profile[0]) / size[0], ((double) profile[1]) / size[1], ((double) profile[2]) / size[2]);
#endif
		return df;	
	};
public:
	SmartMedian2(PClip clip, PClip clip2, int *_xradius, int *_yradius, int *_limit, int _weight, bool planar) : GenericVideoFilter(clip), PlanarAccess(vi), child2(clip2), weight(_weight)
	{
		CompareVideoInfo(vi, child2->GetVideoInfo(), "SmartMedian2"); 
		if(planes == 0) AVSenvironment->ThrowError("SmartMedian2: only planar color spaces are allowed");
	
		int i = planes;
		do
		{
#ifdef	PROFILE_VERSION
			size[i] = width[i] * height[i];
#endif
			xradius[i] = _xradius[i];
			yradius[i] = _yradius[i];
			if( (xradius[i] < 0) || (yradius[i] < 0) )
			{
				pfunction[i] = sm2_do_nothing;
			}
			else
			{
				inner_width[i] = width[i]; inner_height[i] = height[i];
				if( (xradius[i] == 0) || (yradius[i] == 0) )
				{
					pfunction[i] = sm2_copy_plane;
				}
				else
				{
					pfunction[i] = smart_median2;
					int	xr, yr;
					if( ((inner_width[i] -= (xr = 2 * xradius[i] + 1)) <= 0) || ((inner_height[i] -= (yr = 2 * yradius[i] + 1)) <= 0) )
						AVSenvironment->ThrowError("SmartMedian2: width or height of the clip is to small!");
					int	x = xradius[i] + 1;
					int	boxsize = xr * yr;
					int boxsize2 = (boxsize + 1)/ 2;
					int ya = yradius[i] + 1;
					limits[i] = new int *[x] - x;
					int	*mem = new int [x * ya] - ya;
					do
					{
						limits[i][x] = mem;
						mem += ya;
						int y = ya;
						do
						{
							int	k = x * y;
							if( (k = ((k * _limit[i]) + boxsize2) / boxsize) == 0 ) ++k;
							limits[i][x][y] = k;
						} while( ++y <= yr );
					} while( ++x <= xr );
				}
			}
		} while( --i >= 0 );
	}

	~SmartMedian2()
	{
		int i = planes;
		do
		{
			if( pfunction[i] == smart_median2 )
			{
				delete [] (limits[i][xradius[i] + 1] + (yradius[i] + 1));
				delete [] (limits[i] + xradius[i] + 1);
			}
		} while( --i >= 0 );
	}
};

AVSValue __cdecl CreateSmartMedian2(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	enum ARGS { CLIP, CLIP2, RADIUS, PIXELS, RADIUS_Y, YRADIUS_Y, PIXELS_Y, RADIUS_U, YRADIUS_U, PIXELS_U
					, RADIUS_V, YRADIUS_V, PIXELS_V, WEIGHT, PLANAR};

	int	xradius[3], yradius[3], limits[3];
	int radius = args[RADIUS].AsInt(DEFAULT_RADIUS);
	int weight = args[WEIGHT].AsInt(1);
	if( weight < 0 ) weight = 0;
	if( weight > 100 ) weight = 100;
	int	factor = 1 + weight;
	int limit = args[PIXELS].AsInt(factor*(4*radius + 2));
	
	int i = 2, j = 6; 
	do
	{
		xradius[i] = args[RADIUS_Y + j].AsInt(radius);
		yradius[i] = args[YRADIUS_Y + j].AsInt(xradius[i]);
		limits[i] = args[PIXELS + j].AsInt(args[RADIUS_Y + j].Defined() || args[RADIUS_Y + j].Defined() ? factor * (4 * min(xradius[i], yradius[i]) + 2) : limit);
		int maxsize = factor * (2 * xradius[i] + 1) * (2 * yradius[i] + 1);
		if( limits[i] > maxsize ) limits[i] = maxsize;
		if( limits[i] <= 0 ) limits[i] = 1;
		j -= 3;
	} while( --i >= 0 );
	
	return new SmartMedian2(args[CLIP].AsClip(), args[CLIP2].AsClip(), xradius, yradius, limits, weight, args[PLANAR].AsBool(false));
}

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif	
		rank_repair_copy(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, int w, int h, int xradius, int yradius)
{
	AVSenvironment->BitBlt(dst, dpitch, src1, spitch1, w, h);
#ifdef	PROFILE_VERSION
	return 0;
#endif
}

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif	
		restore_chroma(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, int w, int h, int xradius, int yradius)
{
	AVSenvironment->BitBlt(dst, dpitch, src0, spitch0, w, h);
#ifdef	PROFILE_VERSION
	return 0;
#endif
}

#ifdef	PROFILE_VERSION
unsigned
#else
void
#endif	
		rank_repair_nothing(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, int w, int h, int xradius, int yradius)
{
#ifdef	PROFILE_VERSION
	return 0;
#endif
}

class	RankRepair : public GenericVideoFilter, public PlanarAccess
{
	PClip	child2;
#ifdef	PROFILE_VERSION
	unsigned	size[3];
unsigned
#else
void
#endif	
			(*pfunction[3])(BYTE *dst, int dpitch, const BYTE *src0, int spitch0, const BYTE *src1, int spitch1, int w, int h, int xradius, int yradius);
	int	xradius[3], yradius[3];
	int	inner_width[3], inner_height[3];
	int weight;
	
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
#ifdef	PROFILE_VERSION
		unsigned	profile[3];
#endif
		PVideoFrame df = env->NewVideoFrame(vi);
		PVideoFrame sf0 = child2->GetFrame(n, env);
		PVideoFrame sf1 = child->GetFrame(n, env);
		
		int	i = planes;
		do
		{
#ifdef	PROFILE_VERSION
			profile[i] =
#endif
			pfunction[i](GetWritePtr(df, i), GetPitch(df, i), GetReadPtr(sf0, i), GetPitch(sf0, i), GetReadPtr(sf1, i), GetPitch(sf1, i), inner_width[i], inner_height[i], xradius[i], yradius[i]);
		} while( --i >= 0 );
#ifdef	PROFILE_VERSION
		debug_printf("[%i] TemporalSmartMedian Profile: %f, %f, %f\n", n, ((double) profile[0]) / size[0], ((double) profile[1]) / size[1], ((double) profile[2]) / size[2]);
#endif
		return df;	
	};
public:
	RankRepair(PClip clip, PClip clip2, int *_xradius, int *_yradius, bool _restore_chroma, bool planar) : GenericVideoFilter(clip), PlanarAccess(vi), child2(clip2)
	{
		CompareVideoInfo(vi, child2->GetVideoInfo(), "RankRepair"); 
		if(planes == 0) AVSenvironment->ThrowError("RankRepair: only planar color spaces are allowed");
	
		int i = planes;
		do
		{
#ifdef	PROFILE_VERSION
			size[i] = width[i] * height[i];
#endif
			xradius[i] = _xradius[i];
			yradius[i] = _yradius[i];
			if( (xradius[i] < 0) || (yradius[i] < 0) )
			{
				pfunction[i] = rank_repair_nothing;
			}
			else
			{
				inner_width[i] = width[i]; inner_height[i] = height[i];
				if( (xradius[i] == 0) || (yradius[i] == 0) )
				{
					pfunction[i] = rank_repair_copy;
				}
				else
				{
					pfunction[i] = rank_repair;
					int	xr, yr;
					if( ((inner_width[i] -= (xr = 2 * xradius[i] + 1)) <= 0) || ((inner_height[i] -= (yr = 2 * yradius[i] + 1)) <= 0) )
						AVSenvironment->ThrowError("RankRepair: width or height of the clip is to small!");
				}
			}
		} while( --i >= 0 );
		if( vi.IsYUV() && _restore_chroma )
		{
			pfunction[1] = pfunction[2] = restore_chroma;
		}
	}
};

AVSValue __cdecl CreateRankRepair(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	enum ARGS { CLIP, CLIP2, RADIUS, RADIUS_Y, YRADIUS_Y, RADIUS_U, YRADIUS_U, RADIUS_V, YRADIUS_V, RESTORE_CHROMA, PLANAR};

	int	xradius[3], yradius[3];
	int radius = args[RADIUS].AsInt(DEFAULT_RADIUS);
	
	int i = 2, j = 4; 
	do
	{
		xradius[i] = args[RADIUS_Y + j].AsInt(radius);
		yradius[i] = args[YRADIUS_Y + j].AsInt(xradius[i]);
		j -= 2;
	} while( --i >= 0 );
	
	return new RankRepair(args[CLIP].AsClip(), args[CLIP2].AsClip(), xradius, yradius, args[RESTORE_CHROMA].AsBool(false), args[PLANAR].AsBool(false));
}


#ifndef	LOOKUP
#define	LOOKUP
#endif

const AVS_Linkage* AVS_linkage = nullptr;

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment * env, const AVS_Linkage* const vectors) {
  AVS_linkage = vectors;
	AVSenvironment = env;
#ifdef	LOOKUP
	/* initialise ltable */
	int	i = TABLE_MAX + 1;
	do
	{
		ltable[TABLE_MAX + i] = 0;
	} while ( --i );
	i = TABLE_MAX;
	do
	{
		ltable[i] = 1;
	} while ( --i >= 0 );
#endif
#ifdef	DEBUG_NAME
	env->AddFunction("DQuantile", "c[radius]i[rank]i[radius_y]i[yradius_y]i[rank_y]i[radius_u]i[yradius_u]i[rank_u]i[radius_v]i[yradius_v]i[rank_v]i[planar]b", CreateQuantile, 0);
	env->AddFunction("DSmartMedian", "c[radius]i[pixels]i[radius_y]i[yradius_y]i[pixels_y]i[radius_u]i[yradius_u]i[pixels_u]i[radius_v]i[yradius_v]i[pixels_v]i[planar]b", CreateSmartMedian, 0);
	env->AddFunction("DRemoveGrainHD", "c[repair]c[radius]i[rank]i[radius_y]i[yradius_y]i[rank_y]i[urank_y]i[radius_u]i[yradius_u]i[rank_u]i[urank_u]i[radius_v]i[yradius_v]i[rank_v]i[urank_v]i[planar]b", CreateRemoveGrainHD, 0);
	env->AddFunction("DTemporalRemoveGrainHD", "c[repair]c[radius]i[rank]i[radius_y]i[yradius_y]i[rank_y]i[urank_y]i[radius_u]i[yradius_u]i[rank_u]i[urank_u]i[radius_v]i[yradius_v]i[rank_v]i[urank_v]i[weight]i[planar]b", CreateTemporalRemoveGrainHD, 0);
	env->AddFunction("DTemporalSmartMedian", "c[radius]i[pixels]i[radius_y]i[yradius_y]i[pixels_y]i[radius_u]i[yradius_u]i[pixels_u]i[radius_v]i[yradius_v]i[pixels_v]i[weight]i[planar]b", CreateTemporalSmartMedian, 0);
	env->AddFunction("DSmartMedian2", "cc[radius]i[pixels]i[radius_y]i[yradius_y]i[pixels_y]i[radius_u]i[yradius_u]i[pixels_u]i[radius_v]i[yradius_v]i[pixels_v]i[weight]i[planar]b", SmartMedian2, 0);
	env->AddFunction("DRankRepair", "cc[radius]i[radius_y]i[yradius_y]i[radius_u]i[yradius_u]i[radius_v]i[yradius_v]i[restore_chroma]b[planar]b", CreateRankRepair, 0);
	debug_printf(LOGO);
#else
	env->AddFunction("Quantile", "c[radius]i[rank]i[radius_y]i[yradius_y]i[rank_y]i[radius_u]i[yradius_u]i[rank_u]i[radius_v]i[yradius_v]i[rank_v]i[planar]b", CreateQuantile, 0);
	env->AddFunction("SmartMedian", "c[radius]i[pixels]i[radius_y]i[yradius_y]i[pixels_y]i[radius_u]i[yradius_u]i[pixels_u]i[radius_v]i[yradius_v]i[pixels_v]i[planar]b", CreateSmartMedian, 0);
	env->AddFunction("RemoveGrainHD", "c[repair]c[radius]i[rank]i[radius_y]i[yradius_y]i[rank_y]i[urank_y]i[radius_u]i[yradius_u]i[rank_u]i[urank_u]i[radius_v]i[yradius_v]i[rank_v]i[urank_v]i[planar]b", CreateRemoveGrainHD, 0);
	env->AddFunction("TemporalRemoveGrainHD", "c[repair]c[radius]i[rank]i[radius_y]i[yradius_y]i[rank_y]i[urank_y]i[radius_u]i[yradius_u]i[rank_u]i[urank_u]i[radius_v]i[yradius_v]i[rank_v]i[urank_v]i[weight]i[planar]b", CreateTemporalRemoveGrainHD, 0);
	env->AddFunction("TemporalSmartMedian", "c[radius]i[pixels]i[radius_y]i[yradius_y]i[pixels_y]i[radius_u]i[yradius_u]i[pixels_u]i[radius_v]i[yradius_v]i[pixels_v]i[weight]i[planar]b", CreateTemporalSmartMedian, 0);
	env->AddFunction("SmartMedian2", "cc[radius]i[pixels]i[radius_y]i[yradius_y]i[pixels_y]i[radius_u]i[yradius_u]i[pixels_u]i[radius_v]i[yradius_v]i[pixels_v]i[weight]i[planar]b", CreateSmartMedian2, 0);
	env->AddFunction("RankRepair", "cc[radius]i[radius_y]i[yradius_y]i[radius_u]i[yradius_u]i[radius_v]i[yradius_v]i[restore_chroma]b[planar]b", CreateRankRepair, 0);
#endif
	return NULL;
}