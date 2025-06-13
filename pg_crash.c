#include <unistd.h>

#include "c.h"
#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "access/twophase.h"
#include "catalog/pg_control.h"
#include "postmaster/bgworker.h"
#include "storage/proc.h"
#include "utils/guc.h"

PG_MODULE_MAGIC;

extern void _PG_init(void);
PGDLLEXPORT extern void crash_worker_main(Datum main_arg);

static int	signal_delay;
static char *crash_signals;
static List *signals = NIL;

void
_PG_init(void)
{
	BackgroundWorker worker;

	DefineCustomIntVariable(
		"crash.delay",
		"Sleep time (in seconds) of the crash worker.",
		"Sleep this many seconds before sending the next signal.",
		&signal_delay,
		1, 1, INT_MAX,
		PGC_POSTMASTER,
		GUC_UNIT_S,
		NULL, NULL, NULL);

	DefineCustomStringVariable(
		"crash.signals",
		"A list of signals to choose from.",
		"Random signal is chosen out of the ones contained in the set (space-separated).",
		&crash_signals,
		"15",
		PGC_POSTMASTER,
		0,
		NULL, NULL, NULL);

	worker.bgw_flags = BGWORKER_SHMEM_ACCESS;
	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
	worker.bgw_restart_time = 0;
	sprintf(worker.bgw_type, "crash worker");
	sprintf(worker.bgw_name, "crash worker");
	sprintf(worker.bgw_library_name, "pg_crash");
	sprintf(worker.bgw_function_name, "crash_worker_main");
	worker.bgw_main_arg = (Datum) 0;
	worker.bgw_notify_pid = 0;
	RegisterBackgroundWorker(&worker);
}

static volatile sig_atomic_t got_sigterm = false;

static void
crash_worker_sigterm(SIGNAL_ARGS)
{
	int			save_errno = errno;

	got_sigterm = true;
	SetLatch(MyLatch);

	errno = save_errno;
}

void
crash_worker_main(Datum main_arg)
{
	uint32	nprocs = MaxBackends + NUM_AUXILIARY_PROCS + max_prepared_xacts;
	PGPROC	*procs = ProcGlobal->allProcs;

	pqsignal(SIGTERM, crash_worker_sigterm);
	BackgroundWorkerUnblockSignals();

	if (signals == NIL)
	{
		char	*c, *start = NULL;

		/* Parse the list of signals. */
		c = crash_signals;
		while (true)
		{
			/* Looking for the next value? */
			if (isspace(*c) && start == NULL)
			{
				c++;
				/* Done? */
				if (*c == '\0')
					break;
				continue;
			}

			if (start == NULL)
			{
				start = c++;
				continue;
			}
			else if (isspace(*c) || *c == '\0')
			{
				Size	len = c - start;
				char	*str = pnstrdup(start, len);
				long int		nr;

				errno = 0;
				nr = strtol(str, NULL, 10);

				if (errno != 0)
					ereport(ERROR,
							(errmsg("\"%s\" is not a valid integer value",
									str)));
				pfree(str);
				signals = lappend_int(signals, nr);

				if (*c == '\0')
					break;

				start = NULL;
			}
			c++;
		}

		if (list_length(signals) == 0)
			ereport(ERROR, (errmsg("no signals specified")));
	}

	ereport(LOG,
			(errmsg("pg_crash background worker started, crash.delay = %d, crash.signals = '%s'",
					signal_delay, crash_signals)));

	for (;;)
	{
		long int	n;
		int	i, j;
		int	rc;
		int	signal;

		/* wait for signal_delay seconds */
		ResetLatch(MyLatch);

		rc = WaitLatch(MyLatch, WL_LATCH_SET | WL_POSTMASTER_DEATH |
					   WL_TIMEOUT, signal_delay * 1000L, 0);
		if (rc & WL_POSTMASTER_DEATH)
			break;

		if (got_sigterm)
			break;

		/* Select signal. */
		n = rand() % list_length(signals);
		signal = list_nth_int(signals, n);

		/* Select backend. */
		n = rand() % nprocs;
		j = 0;
		for (i = 0;; i++)
		{
			PGPROC	*proc = &procs[i % nprocs];

			/*
			 * Do not send signals to the bgworker, otherwise it won't be
			 * restarted after a crash.
			 */
			if (proc->pid > 0 && proc->pid != MyProcPid)
			{
				if (j == n)
				{
					/* Send the signal. */
					ereport(DEBUG1,
							(errmsg("sending signal %d to process %d",
									signal, proc->pid)));
					kill(proc->pid, signal);
					break;
				}
				j++;
			}
		}
	}
}
