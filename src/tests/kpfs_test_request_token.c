/*
   (c) Copyright 2012  Tao Yu

   All rights reserved.

   Written by Tao Yu <yut616@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "kpfs.h"
#include "kpfs_oauth.h"
#include "kpfs_conf.h"

static void kpfs_test_usage(char *argv0)
{
	if (NULL == argv0)
		return;
	fprintf(stderr, "%s version: %s\n", argv0, KPFS_VERSION);
	fprintf(stderr, "usage:  %s -c <path of kpfs.conf> \n", argv0);
	fprintf(stderr, "\t -c <path of kpfs.conf> \tset config file for kpfs, it is json format.\n");
	fprintf(stderr, "\t -h \t --help \t\tthis usage.\n");
	fprintf(stderr, "\t e.g.: %s -c /etc/kpfs.conf\n", argv0);
	fprintf(stderr, "\t you should set mount point in kpfs.conf\n");
}

int main(int argc, char *argv[])
{
	kpfs_ret ret = KPFS_RET_OK;
	struct stat st;

	printf("argc: %d, argv: %s\n", argc, argv[0]);
	if (argc < 3) {
		kpfs_test_usage(argv[0]);
		return -1;
	}
	if (0 == strcmp("-c", argv[1])) {
		if (argv[2] == '\0') {
			kpfs_test_usage(argv[0]);
			return -1;
		}
		if (0 != stat(argv[2], &st)) {
			printf("config file of kpfs (%s) is not existed.\n", argv[2]);
			kpfs_test_usage(argv[0]);
			return -1;
		}
		if (KPFS_RET_OK != kpfs_conf_load(argv[2])) {
			kpfs_test_usage(argv[0]);
			return -1;
		}
	} else {
		kpfs_test_usage(argv[0]);
		return -1;
	}

	kpfs_oauth_init();

	ret = kpfs_oauth_load();
	if (KPFS_RET_OK != ret) {
		printf("This is the first time you use kpfs.\n");
		ret = kpfs_oauth_request_token();
		assert(KPFS_RET_OK == ret);
	}
	return 0;
}
