/*
 * CoreFreq
 * Copyright (C) 2015-2020 CYRIL INGENIERIE
 * Licenses: GPL2
 */

#define _GNU_SOURCE
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sched.h>

#include <X11/Xlib.h>
#ifdef HAVE_XFT
#include <X11/Xft/Xft.h>
#endif

#include "bitasm.h"
#include "coretypes.h"
#include "corefreq.h"
#include "corefreq-gui-lib.h"
#include "corefreq-gui.h"

SERVICE_PROC localService = {.Proc = -1};

int ClientFollowService(SERVICE_PROC *pSlave, SERVICE_PROC *pMaster, pid_t pid)
{
	if (pSlave->Proc != pMaster->Proc) {
		pSlave->Proc = pMaster->Proc;

		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(pSlave->Core, &cpuset);
		if (pSlave->Thread != -1)
			CPU_SET(pSlave->Thread, &cpuset);

		return (sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset));
	}
	return (0);
}

void Build_Processor(XWINDOW *W)
{
	const size_t len = strlen(W->A->M.RO(Shm)->Proc.Brand);
	/* Reset the color context.					*/
	SetFG(W, SMALL, W->color.foreground);
	/* Processor specification.					*/
	DrawStr(W, B, LARGE,
		abs(W->width - (One_Char_Width(W, LARGE) * len)) / 2,
		One_Char_Height(W, LARGE),
		W->A->M.RO(Shm)->Proc.Brand, len);
}

void Build_CPU_Frequency(XWINDOW *W)
{
	const int	x = One_Char_Width(W, SMALL),
			y = Twice_Char_Height(W, LARGE),
			w = 42 * One_Char_Width(W, SMALL);
	/* Columns header						*/
	DrawStr(W, B, MEDIUM, x, y, "CPU", 3);

	DrawStr(W, B, MEDIUM, x + w - ((8+1) * One_Char_Width(W, SMALL)), y,
		"Freq[Mhz]", 9 );
}

void BuildLayout(uARG *U)
{
	XWINDOW *walker = U->A->W;
	while (walker != NULL) {
		Build_Processor(walker);
		Build_CPU_Frequency(walker);
		walker = GetNext(walker);
	}
}

void Draw_CPU_Frequency(XWINDOW *W,
			const unsigned int cpu, const struct FLIP_FLOP *CFlop)
{
	const int	x = One_Char_Width(W, SMALL),
			y = One_Half_Char_Height(W, LARGE),
			w = 42 * One_Char_Width(W, SMALL),
			h = One_Half_Char_Height(W, SMALL),

		r = ((w - Twice_Char_Width(W, SMALL)) * CFlop->Relative.Ratio)
			/ W->A->M.RO(Shm)->Cpu[cpu].Boost[BOOST(1C)],

		p = y + (Twice_Char_Height(W, SMALL) * (cpu + 1));

	char str[16];
	snprintf(str, 16, "%03u%7.2f", cpu, CFlop->Relative.Freq);

    if (CFlop->Relative.Ratio >= W->A->M.RO(Shm)->Cpu[cpu].Boost[BOOST(MAX)]) {
	SetFG(W, SMALL, _COLOR_BAR);
    } else {
	SetFG(W);
    }
	FillRect(W, F, SMALL, x, p, r, h);

	SetFG(W, SMALL, _COLOR_TEXT);

	DrawStr(W, F, SMALL, x, p + One_Char_Height(W, SMALL), str, 3);

	DrawStr(W, F, SMALL,
		x + w - ((7+1) * One_Char_Width(W, SMALL)),
		p + One_Char_Height(W, SMALL),
		&str[3], 7);
}

void DrawLayout(uARG *U)
{
	unsigned int cpu;
    for (cpu = 0; cpu < U->A->M.RO(Shm)->Proc.CPU.Count; cpu++)
    {
	struct FLIP_FLOP *CFlop = \
		&U->A->M.RO(Shm)->Cpu[cpu].FlipFlop[
			!U->A->M.RO(Shm)->Cpu[cpu].Toggle
		];

	XWINDOW *walker = U->A->W;
	while (walker != NULL) {
		Draw_CPU_Frequency(walker, cpu, CFlop );
		walker = GetNext(walker);
	}
    }
}

void MapLayout(xARG *A)
{
	XCopyArea(A->display, A->W->pixmap.B, A->W->pixmap.F, A->W->gc[SMALL],
			0, 0, A->W->width, A->W->height, 0, 0);
}

void FlushLayout(xARG *A)
{
	XCopyArea(A->display, A->W->pixmap.F, A->W->window, A->W->gc[SMALL],
			0, 0, A->W->width, A->W->height, 0, 0);

	XFlush(A->display);
}

static void *DrawLoop(void *uArg)
{
	uARG *U = (uARG *) uArg;

	pthread_setname_np(U->TID.Drawing, "corefreq-gui-dw");

	ClientFollowService(&localService, &U->A->M.RO(Shm)->Proc.Service, 0);

  while (!BITVAL(U->Shutdown, SYNC))
  {
    if (BITCLR(LOCKLESS, U->A->M.RW(Shm)->Proc.Sync, SYNC1) == 0) {
	nanosleep(&U->A->M.RO(Shm)->Sleep.pollingWait, NULL);
    } else {
	Paint(U, False, True, True, True);
    }
    if (BITCLR(LOCKLESS, U->A->M.RW(Shm)->Proc.Sync, NTFY1)) {
	ClientFollowService(&localService,&U->A->M.RO(Shm)->Proc.Service,0);
    }
}
	return(NULL);
}

void *EventLoop(uARG *U)
{
	while (!BITVAL(U->Shutdown, SYNC))
	{
		switch (EventGUI(U->A)) {
		case GUI_NONE:
			break;
		case GUI_EXIT:
			BITSET(LOCKLESS, U->Shutdown, SYNC);
			break;
		case GUI_BUILD:
			break;
		case GUI_MAP:
			break;
		case GUI_DRAW:
			break;
		case GUI_FLUSH:
			Paint(U, False, False, False, True);
			break;
		case GUI_PAINT:
			Paint(U, True, True, False, True);
			break;
		}
	}
	return (NULL);
}

static void *Emergency(void *uArg)
{
	uARG *U=(uARG *) uArg;

	pthread_setname_np(U->TID.SigHandler, "corefreq-gui-sg");

	ClientFollowService(&localService, &U->A->M.RO(Shm)->Proc.Service, 0);

	int caught = 0;
    while (!BITVAL(U->Shutdown, SYNC) && !sigwait(&U->TID.Signal, &caught))
    {
      if (BITVAL(LOCKLESS, U->A->M.RW(Shm)->Proc.Sync, NTFY1)) {
	ClientFollowService(&localService,&U->A->M.RO(Shm)->Proc.Service,0);
      }
	switch (caught) {
	case SIGINT:
	case SIGQUIT:
	case SIGUSR1:
	case SIGTERM: {
		XClientMessageEvent E = {
			.type	= ClientMessage,
			.serial = 0,
			.send_event = False,
			.display = U->A->display,
			.window = U->A->W->window,
			.message_type = U->A->atom[0],
			.format = 32,
			.data	= { .l = {U->A->atom[0]} }
		};
		XSendEvent(U->A->display, U->A->W->window, 0,0, (XEvent *) &E);
		XFlush(U->A->display);
	}
		break;
	}
    }
	return (NULL);
}

OPTION Options[] = {
	{ .opt="--help"  , .fmt=NULL , .dsc="\t\tPrint out this message"},
	{ .opt="--small" , .fmt="%s" , .dsc="\t\tX Small Font"		},
	{ .opt="--medium", .fmt="%s" , .dsc="\tX Medium Font"		},
	{ .opt="--large" , .fmt="%s" , .dsc="\t\tX Large Font"		},
	{ .opt="--Xacl"  , .fmt="%c" , .dsc="\t\tXEnableAccessControl"	},
	{ .opt="--root"  , .fmt="%c" , .dsc="\t\tUse the root window"	},
	{ .opt="--fg"    , .fmt="%lx", .dsc="\t\tForeground color"	},
	{ .opt="--bg"    , .fmt="%lx", .dsc="\t\tbackground color"	}
};

OPTION *Option(int jdx, uARG *U)
{
	OPTION variable[] = {
		{ .var = NULL					},
		{ .var = U->A->font[SMALL].name 		},
		{ .var = U->A->font[MEDIUM].name		},
		{ .var = U->A->font[LARGE].name 		},
		{ .var = &U->A->Xacl,				},
		{ .var = &U->A->Xroot,				},
		{ .var = &U->A->W->color.foreground		},
		{ .var = &U->A->W->color.background		}
	};
	const size_t zdx = sizeof(Options) / sizeof(OPTION);

	if (jdx < zdx) {
		Options[jdx].var = variable[jdx].var;
		return (&Options[jdx]);
	} else {
		return (NULL);
	}
}

void Help(uARG *U)
{
	const size_t zdx = sizeof(Options) / sizeof(OPTION);
	int jdx = strlen(__FILE__) - 2;

	printf( "CoreFreq."					\
		"  Copyright (C) 2015-2020 CYRIL INGENIERIE\n\n"\
		"Usage:\t%.*s [--option <arguments>]\n", jdx, __FILE__ );

	for (jdx = 0; jdx < zdx; jdx++) {
		OPTION *pOpt = Option(jdx, U);
		if (pOpt != NULL) {
			printf("\t%s%s\n", pOpt->opt, pOpt->dsc);
		}
	}
	printf( "\nExit status:\n"				\
		"\t0\tSUCCESS\t\tSuccessful execution\n"	\
		"\t1\tSYNTAX\t\tCommand syntax error\n" 	\
		"\t2\tSYSTEM\t\tAny system issue\n"		\
		"\t3\tDISPLAY\t\tDisplay setup error\n" 	\
		"\t4\tVERSION\t\tMismatch API version\n"	\
		"\nReport bugs to labs[at]cyring.fr\n" );
}

int Command(int argc, char *argv[], uARG *U)
{
	GUI_REASON rc = GUI_SUCCESS;

    if (argc > 1)
    {
	OPTION *pOpt = NULL;
	int idx = 1, jdx;

	while ((rc == GUI_SUCCESS) && (idx < argc))
	{
	    for (jdx = 0;
		(rc == GUI_SUCCESS) && ((pOpt = Option(jdx, U)) != NULL);
			jdx++)
	    {
		ssize_t len = strlen(pOpt->opt) - strlen(argv[idx]);
		if (len == 0)
		{
			len = strlen(pOpt->opt);
		    if (strncmp(pOpt->opt, argv[idx], len) == 0)
		    {
			if ((++idx < argc) && (pOpt->var != NULL)) {
			    if (sscanf(argv[idx++], pOpt->fmt, pOpt->var) != 1)
			    {
				rc = GUI_SYNTAX;
			    }
			} else {
				rc = GUI_SYNTAX;
			}
			break;
		    }
		}
	    }
	    if((jdx == sizeof(Options) / sizeof(OPTION)) && (rc == GUI_SUCCESS))
	    {
		rc = GUI_SYNTAX;
	    }
	}
    }
	return (rc);
}

int main(int argc, char *argv[])
{
	GUI_REASON rc = GUI_SUCCESS;

	uARG U = {
		.Shutdown = 0x0,
		.FD = { .ro = -1, .rw = -1 },
		.TID = { .SigHandler = 0, .Drawing = 0 },

		.A = AllocGUI()
	};

  if ((U.A->W = AllocWidget(U.A)) != NULL) {
	rc = Command(argc, argv, &U);
  } else {
	rc = GUI_SYSTEM;
  }
  if ((rc == GUI_SUCCESS)
	&& (XInitThreads() != 0)
		&& ((rc = OpenDisplay(U.A)) == GUI_SUCCESS))
  {
    if	(	((U.FD.ro = shm_open(RO(SHM_FILENAME), O_RDONLY,
				 S_IRUSR|S_IWUSR
				|S_IRGRP|S_IROTH)) != -1)
    &&		((U.FD.rw = shm_open(RW(SHM_FILENAME), O_RDWR,
				 S_IRUSR|S_IWUSR
				|S_IRGRP|S_IWGRP
				|S_IROTH|S_IWOTH)) != -1) )
    {
	struct stat stat_ro = {0}, stat_rw = {0};

	if((fstat(U.FD.ro, &stat_ro) != -1) && (fstat(U.FD.rw, &stat_rw) != -1))
	{
	  if (	((U.A->M.RO(Shm) = mmap( NULL, stat_ro.st_size,
					PROT_READ, MAP_SHARED,
					U.FD.ro, 0 )) != MAP_FAILED)
	  &&	((U.A->M.RW(Shm) = mmap( NULL, stat_rw.st_size,
					PROT_READ|PROT_WRITE, MAP_SHARED,
					U.FD.rw, 0 )) != MAP_FAILED) )
	  {
	    if (CHK_FOOTPRINT(U.A->M.RO(Shm)->FootPrint,MAX_FREQ_HZ,
							CORE_COUNT,
							TASK_ORDER,
							COREFREQ_MAJOR,
							COREFREQ_MINOR,
							COREFREQ_REV))
	    {
		ClientFollowService(	&localService,
					&U.A->M.RO(Shm)->Proc.Service, 0 );

		RING_WRITE_SUB_CMD(	SESSION_GUI, U.A->M.RW(Shm)->Ring[1],
					COREFREQ_SESSION_APP, getpid() );

		rc = StartWidget(U.A, GEOMETRY_WIDTH, GEOMETRY_HEIGHT, 
				_BACKGROUND_GLOBAL, _FOREGROUND_GLOBAL,
				"CoreFreq");

		if (rc == GUI_SUCCESS)
		{
			sigemptyset(&U.TID.Signal);
			sigaddset(&U.TID.Signal, SIGINT);  /* [CTRL] + [C] */
			sigaddset(&U.TID.Signal, SIGQUIT);
			sigaddset(&U.TID.Signal, SIGUSR1);
			sigaddset(&U.TID.Signal, SIGTERM);

		    if (pthread_sigmask(SIG_BLOCK, &U.TID.Signal, NULL) == 0) {
			pthread_create(&U.TID.SigHandler, NULL, Emergency, &U);
		    } else {
			rc = GUI_SYSTEM; /* Run without it however... */
		    }
		    if (pthread_create(&U.TID.Drawing, NULL, DrawLoop, &U) == 0)
		    {
			EventLoop(&U);

			pthread_join(U.TID.Drawing, NULL);
		    } else {
			rc = GUI_SYSTEM;
		    }
		    if (U.TID.SigHandler != 0)
		    {
			pthread_kill(U.TID.SigHandler, SIGUSR1);
			pthread_join(U.TID.SigHandler, NULL);
		    }
			StopWidget(U.A);
		}
		RING_WRITE_SUB_CMD(	SESSION_GUI, U.A->M.RW(Shm)->Ring[1],
					COREFREQ_SESSION_APP, (pid_t) 0 );
	    } else {
		rc = GUI_VERSION;
	    }
		munmap(U.A->M.RO(Shm), stat_ro.st_size);
		munmap(U.A->M.RW(Shm), stat_rw.st_size);
	  } else {
		rc = GUI_SYSTEM;
	  }
	} else {
		rc = GUI_SYSTEM;
	}
    } else {
	rc = GUI_SYSTEM;
    }
	CloseDisplay(U.A);
  }
  if ((rc != GUI_SUCCESS) && (U.A != NULL)) {
	Help(&U);
  }
	FreeWidget(U.A->W);
	FreeGUI(U.A);
	return (rc);
}
