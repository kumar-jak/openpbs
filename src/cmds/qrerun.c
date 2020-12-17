/*
 * Copyright (C) 1994-2020 Altair Engineering, Inc.
 * For more information, contact Altair at www.altair.com.
 *
 * This file is part of both the OpenPBS software ("OpenPBS")
 * and the PBS Professional ("PBS Pro") software.
 *
 * Open Source License Information:
 *
 * OpenPBS is free software. You can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * OpenPBS is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Commercial License Information:
 *
 * PBS Pro is commercially licensed software that shares a common core with
 * the OpenPBS software.  For a copy of the commercial license terms and
 * conditions, go to: (http://www.pbspro.com/agreement.html) or contact the
 * Altair Legal Department.
 *
 * Altair's dual-license business model allows companies, individuals, and
 * organizations to create proprietary derivative works of OpenPBS and
 * distribute them - whether embedded or bundled with other software -
 * under a commercial license agreement.
 *
 * Use of Altair's trademarks, including but not limited to "PBS™",
 * "OpenPBS®", "PBS Professional®", and "PBS Pro™" and Altair's logos is
 * subject to Altair's trademark licensing policies.
 */

/**
 * @file	qrerun.c
 * @brief
 * 	qrerun - (PBS) rerun batch job
 *
 * @author	   	Terry Heidelberg
 * 				Livermore Computing
 *
 * @author     	Bruce Kelly
 * 				National Energy Research Supercomputer Center
 *
 * @author     	Lawrence Livermore National Laboratory
 * 				University of California
 */

#include "cmds.h"
#include "pbs_ifl.h"
#include <pbs_config.h>   /* the master config generated by configure */
#include <pbs_version.h>


int
main(int argc, char **argv, char **envp) /* qrerun */
{
	int any_failed=0;

	char job_id[PBS_MAXCLTJOBID];       /* from the command line */

	char job_id_out[PBS_MAXCLTJOBID];
	char server_out[PBS_MAXSERVERNAME];
	char rmt_server[MAXSERVERNAME];
	char *extra = NULL;
	char *force = "force";
	int  i;
	static char usage[]="usage: qrerun [-W force] job_identifier...\n";
	static char usag2[]="       qrerun --version\n";

	/*test for real deal or just version and exit*/

	PRINT_VERSION_AND_EXIT(argc, argv);

	if (initsocketlib())
		return 1;

	while ((i = getopt(argc, argv, "W:")) != EOF) {
		switch (i) {
			case 'W':
				if (strcmp(optarg, force) == 0)
					extra = force;
				else {
					fprintf(stderr, "%s", usage);
					fprintf(stderr, "%s", usag2);
					exit(2);
				}
				break;
			default:
				fprintf(stderr, "%s", usage);
				fprintf(stderr, "%s", usag2);
				exit(2);
		}
	}


	if (optind == argc) {
		fprintf(stderr, "%s", usage);
		fprintf(stderr, "%s", usag2);
		exit(2);
	}

	/*perform needed security library initializations (including none)*/

	if (CS_client_init() != CS_SUCCESS) {
		fprintf(stderr, "qrerun: unable to initialize security library.\n");
		exit(1);
	}

	for (; optind < argc; optind++) {
		int connect;
		int stat=0;
		int located = FALSE;

		pbs_strncpy(job_id, argv[optind], sizeof(job_id));
		if (get_server(job_id, job_id_out, server_out)) {
			fprintf(stderr, "qrerun: illegally formed job identifier: %s\n", job_id);
			any_failed = 1;
			continue;
		}
cnt:
		connect = cnt2server(server_out);
		if (connect <= 0) {
			fprintf(stderr, "qrerun: cannot connect to server %s (errno=%d)\n",
				pbs_server, pbs_errno);
			any_failed = pbs_errno;
			continue;
		}

		stat = pbs_rerunjob(connect, job_id_out, extra);
		if (stat && (pbs_errno != PBSE_UNKJOBID)) {
			prt_job_err("qrerun", connect, job_id_out);
			any_failed = pbs_errno;
		} else if (stat && (pbs_errno == PBSE_UNKJOBID) && !located) {
			located = TRUE;
			if (locate_job(job_id_out, server_out, rmt_server)) {
				pbs_disconnect(connect);
				pbs_strncpy(server_out, rmt_server, sizeof(server_out));
				goto cnt;
			}
			prt_job_err("qrerun", connect, job_id_out);
			any_failed = pbs_errno;
		}

		pbs_disconnect(connect);
	}

	/*cleanup security library initializations before exiting*/
	CS_close_app();

	exit(any_failed);
}
