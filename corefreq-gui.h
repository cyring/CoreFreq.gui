/*
 * CoreFreq
 * Copyright (C) 2015-2022 CYRIL INGENIERIE
 * Licenses: GPL2
 */

typedef struct {
	void	*var;
	char	*opt,
		*fmt,
		*dsc;
} OPTION;

typedef struct {
	Bit64 Shutdown	__attribute__ ((aligned (8)));
	struct
	{
		int		ro, rw;
	} FD;
	struct
	{
		sigset_t	Signal;
		pthread_t	SigHandler,
				Drawing;
	} TID;

	xARG	*A;
} uARG;

/*
	The bouble buffering is as the following sequence:
*/
/* step 1 : draw the static graphics into the background pixmap.	*/
void	BuildLayout(uARG *) ;
/* step 2 : copy the background into the foreground pixmap.		*/
void	MapLayout(xARG *) ;
/* step 3 : add the animated graphics into the foreground pixmap.	*/
void	DrawLayout(uARG *) ;
/* step 4 : copy the foreground into the display window
		(taking care of the scrolling pixmap).			*/
void	FlushLayout(xARG *) ;
/* loop to step 2 to avoid the execution of the building process.	*/

/* All-in-One macro.							*/
#define Paint(U, doBuild, doMap, doDraw, doFlush) {			\
	if (doBuild)	{	BuildLayout(U); 	}		\
	if (doMap)	{	MapLayout(U->A);	}		\
	if (doDraw)	{	DrawLayout(U);		}		\
	if (doFlush)	{	FlushLayout(U->A);	}		\
}
