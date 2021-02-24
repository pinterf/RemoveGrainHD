#ifdef	RG_DEBUG
	int	state;
	int dpitch0 = dpitch;
	BYTE *dst0 = dst;
#endif
#ifdef	LOCALTABLE
	int			table[TABLE_SIZE];
	memset(table, 0, sizeof(int) * TABLE_SIZE);
#endif
	const BYTE	*last = src;
	int	yr = yradius + 1, xr = xradius + 1;

#ifdef	PROFILE_VERSION
	unsigned	profile = 0;
#endif
	
	int	incpitch = spitch - STEP * xr;
	int	sum = 0;
	int lval = src[0];

	INIT_FUNC();

#ifdef	LOOKUP
	int	*lookup = ltable + TABLE_MAX - lval;
#endif
	
	/* upper left corner */
	int	i = yr;
	do
	{
		ADD_LINE();
	} while( --i );
	
	/* compute the quantile */
	PROCESS_TABLE2(0, 0, xr, yr, 1)
	
	/* now we increase yr */
	int y = yradius;
	do
	{
		ADD_LINE();

		/* compute the quantile */
		++yr;
		PROCESS_TABLE2(dpitch, spitch, xr, yr, 2)
	} while( --y );
	
	y = h;
	do
	{
		SUBTRACT_LINE();

		ADD_LINE();

		/* compute the quantile */
		PROCESS_TABLE1(dpitch, spitch, 21)
	} while( --y );
	
	/* now we decrease yr */
	y = yradius;
	do
	{
		SUBTRACT_LINE();

		/* compute the quantile */
		--yr;
		PROCESS_TABLE2(dpitch, spitch, xr, yr, 3)
	} while( --y ); /* we have finished the computation of the first (=left) column of pixels */
	
	/* now we increase xr */
	int	x = xradius;
	do
	{
		spitch = -spitch;
		dpitch = -dpitch;
		last = src + spitch;
		
		ADD_COLUMN();
		src -= STEP * xr;
		
		/* compute the quantile */
		++xr;
		PROCESS_TABLE2(STEP, STEP, xr, yr, 4)

		incpitch += 2*spitch - STEP;

		/* now we increase yr */
		int y = yradius;
		do
		{
			ADD_LINE();
			
			/* compute the quantile */
			++yr;
			PROCESS_TABLE2(dpitch, spitch, xr, yr, 5)
		} while( --y ); 
		
		y = h;
		do
		{
			SUBTRACT_LINE();

			ADD_LINE();
		
			/* compute the quantile */
			PROCESS_TABLE1(dpitch, spitch, 22)
		} while( --y );

		/* now we decrease yr */
		y = yradius;
		do
		{
			SUBTRACT_LINE();
			
			/* compute the quantile */
			--yr;
			PROCESS_TABLE2(dpitch, spitch, xr, yr, 6)
		} while( --y );
	} while( --x );

	/* now we keep xr constant for quite a while */
	x = w;
	do
	{
		SUBTRACT_COLUMN();

		spitch = -spitch;
		dpitch = -dpitch;
		
		ADD_COLUMN();
		src -= STEP * (xr - 1);
		
		/* compute the quantile */
		/* limit = limits[xr][yr]; */
		PROCESS_TABLE1(STEP, STEP, 23)

		incpitch += 2*spitch;

		/* now we increase yr */
		int y = yradius;
		do
		{
			ADD_LINE();
			
			/* compute the quantile */
			++yr;
			PROCESS_TABLE2(dpitch, spitch, xr, yr, 7)
		} while( --y );
		
		y = h;
		do
		{
			SUBTRACT_LINE();

			ADD_LINE();
		
			/* compute the quantile */
			/* limit = limits[xr][yr]; */
			PROCESS_TABLE1(dpitch, spitch, 24)
		} while( --y );

		/* now we decrease yr */
		y = yradius;
		do
		{
			SUBTRACT_LINE();

			/* compute the quantile */
			--yr;
			PROCESS_TABLE2(dpitch, spitch, xr, yr, 8)
		} while( --y );
	} while( --x );
	
	/* now we decrease xr */
	x = xradius;
	do
	{
		src = last - spitch + STEP;
		SUBTRACT_COLUMN();

		spitch = -spitch;
		dpitch = -dpitch;
		
		/* compute the quantile */
		--xr;
		PROCESS_TABLE2(STEP, STEP, xr, yr, 9)

		incpitch += 2*spitch + STEP;

		/* now we increase yr */
		int y = yradius;
		do
		{
			ADD_LINE();
			
			/* compute the quantile */
			++yr;
			PROCESS_TABLE2(dpitch, spitch, xr, yr, 10)
		} while( --y );
		
		y = h;
		do
		{
			SUBTRACT_LINE();

			ADD_LINE();
		
			/* compute the quantile */
			PROCESS_TABLE1(dpitch, spitch, 25)
		} while( --y );

		/* now we decrease yr */
		y = yradius;
		do
		{
			SUBTRACT_LINE();

			/* compute the quantile */
			--yr;
			PROCESS_TABLE2(dpitch, spitch, xr, yr, 11)
		} while( --y );
	} while( --x );
	// check_table(0)
#ifndef	LOCALTABLE
	/* clean the table */
	corner_boxsize -= (sum += limit);
	i = lval;
	int val;
	do 
	{
		val = table[i];
		table[i--] = 0;
	} while( (sum -= val) != 0 );

	do 
	{
		val = table[++lval];
		table[lval] = 0;
	} while( (corner_boxsize -= val) != 0 );
#endif
#ifdef	PROFILE_VERSION
	return profile;
#endif
