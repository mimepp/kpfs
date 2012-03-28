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
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>

#include "kpfs.h"
#include "kpfs_conf.h"

int kpfs_file_log(const char *fmt, ...)
{
	va_list ap;
	char tmp[4096] = { 0 };
	char buf[4096] = { 0 };
	char log_file[KPFS_MAX_PATH] = { 0 };
	int fd = 0;
	char ftime[64] = { 0 };
	struct timeval tv;
	time_t curtime;
	int ret = 0;

	gettimeofday(&tv, NULL);
	curtime = tv.tv_sec;
	strftime(ftime, sizeof(ftime), "%F %T", localtime(&curtime));

	va_start(ap, fmt);
	vsnprintf(tmp, sizeof(tmp), fmt, ap);
	va_end(ap);

	snprintf(buf, sizeof(buf), "[%s] %s", ftime, tmp);
	snprintf(log_file, sizeof(log_file), "%s/%s", kpfs_conf_get_writable_tmp_path(), KPFS_LOG_FILE_NAME);
	fd = open(log_file, O_APPEND | O_WRONLY | O_CREAT, 0666);
	if (-1 == fd)
		return -1;
	ret = write(fd, buf, strlen(buf));
	if (-1 == ret)
		printf("fail to write: %s.\n", log_file);
	close(fd);
	return 0;
}
