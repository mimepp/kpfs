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
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include "kpfs.h"
#include "kpfs_curl.h"

static size_t write_func(void *data, size_t size, size_t nmemb, void *stream)
{
	strncat(stream, data, size * nmemb);
	return size * nmemb;
}

char *kpfs_curl_fetch(const char *url)
{
	char *buf = calloc(KPFS_MAX_BUF, 1);
	CURL *curl = NULL;
	CURLcode ret = -1;

	curl = curl_easy_init();
	if (NULL == curl)
		return NULL;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_func);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
	ret = curl_easy_perform(curl);
	printf("curl_easy_perform return value: %d\n", ret);
	curl_easy_cleanup(curl);
	return buf;
}
