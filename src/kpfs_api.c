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
#include <sys/types.h>
#include <sys/stat.h>
#include <oauth.h>

#include "kpfs.h"
#include "kpfs_oauth.h"
#include "kpfs_api.h"
#include "kpfs_conf.h"
#include "kpfs_util.h"
#include "kpfs_curl.h"

static char *kpfs_api_common_request(char *url)
{
	char *t_key = kpfs_oauth_token_get();
	char *t_secret = kpfs_oauth_token_secret_get();
	char *req_url = NULL;
	char *reply = NULL;
	char *ret = NULL;

	KPFS_FILE_LOG("request url: %s ...\n", url);

	req_url =
	    oauth_sign_url2(url, NULL, OA_HMAC, NULL, (const char *)kpfs_conf_get_consumer_key(), (const char *)kpfs_conf_get_consumer_secret(), t_key,
			    t_secret);
	if (!req_url) {
		goto error_out;
	}

	KPFS_FILE_LOG("real url: %s.\n", req_url);

	reply = oauth_http_get(req_url, NULL);

	if (!reply) {
		goto error_out;
	}

	KPFS_FILE_LOG("response: %s\n", reply);
	ret = reply;

error_out:
	KPFS_SAFE_FREE(req_url);

	return ret;
}

static char *kpfs_api_common_request_with_path(char *url, char *root, char *path)
{
	char *t_key = kpfs_oauth_token_get();
	char *t_secret = kpfs_oauth_token_secret_get();
	char *req_url = NULL;
	char *reply = NULL;
	char *ret = NULL;
	char *path_escape = NULL;
	int malloc_more_room = 100;
	char *url_with_path = NULL;
	int len = 0;

	if (NULL == url || NULL == root || NULL == path)
		return NULL;

	KPFS_FILE_LOG("request url: %s ...\n", url);

	path_escape = oauth_url_escape(path);
	if (NULL == path_escape)
		return NULL;

	len = strlen(url) + strlen(root) + strlen(path_escape) + malloc_more_room;
	url_with_path = calloc(len, 1);
	if (NULL == url_with_path) {
		KPFS_SAFE_FREE(path_escape);
		return NULL;
	}

	snprintf(url_with_path, len, "%s?root=%s&path=%s", url, root, path_escape);
	KPFS_SAFE_FREE(path_escape);

	req_url =
	    oauth_sign_url2(url_with_path, NULL, OA_HMAC, NULL, (const char *)kpfs_conf_get_consumer_key(), (const char *)kpfs_conf_get_consumer_secret(),
			    t_key, t_secret);
	if (!req_url) {
		goto error_out;
	}

	KPFS_FILE_LOG("real url: %s.\n", req_url);

	reply = oauth_http_get(req_url, NULL);

	if (!reply) {
		goto error_out;
	}

	KPFS_FILE_LOG("response: %s\n", reply);
	ret = reply;

error_out:
	KPFS_SAFE_FREE(req_url);

	return ret;
}

char *kpfs_api_account_info()
{
	return kpfs_api_common_request(KPFS_API_ACCOUNT_INFO);
}

static char *kpfs_api_root_get()
{
	return KPFS_API_ROOT;
}

char *kpfs_api_metadata(const char *path)
{
	char fullpath[1024] = { 0 };
	char *path_escape = NULL;

	path_escape = oauth_url_escape(path);
	if (NULL == path_escape)
		return NULL;

	snprintf(fullpath, sizeof(fullpath), "%s%s%s", KPFS_API_METADATA "/", kpfs_api_root_get(), path_escape);
	KPFS_SAFE_FREE(path_escape);
	KPFS_FILE_LOG("fullpath: %s\n", fullpath);
	return kpfs_api_common_request(fullpath);
}

char *kpfs_api_download_link_create(const char *path)
{
	char *t_key = kpfs_oauth_token_get();
	char *t_secret = kpfs_oauth_token_secret_get();
	char *url = KPFS_API_DOWNLOAD_FILE;
	char *req_url = NULL;
	char *ret = NULL;
	char *url_with_path = NULL;
	int len = 0;
	int malloc_more_room = 100;
	char *root = NULL;

	if (NULL == path)
		return ret;

	KPFS_FILE_LOG("request url: %s ...\n", url);
	root = kpfs_api_root_get();
	len = strlen(url) + strlen(root) + strlen(path) + malloc_more_room;
	url_with_path = calloc(len, 1);
	if (NULL == url_with_path)
		return ret;

	snprintf(url_with_path, len, "%s?root=%s&path=%s", url, root, path);

	req_url =
	    oauth_sign_url2(url_with_path, NULL, OA_HMAC, NULL, (const char *)kpfs_conf_get_consumer_key(), (const char *)kpfs_conf_get_consumer_secret(),
			    t_key, t_secret);
	if (!req_url) {
		return ret;
	}

	KPFS_FILE_LOG("download link: %s.\n", req_url);
	ret = req_url;

	return ret;
}

kpfs_ret kpfs_api_create_folder(const char *path)
{
	char *response = NULL;
	response = kpfs_api_common_request_with_path(KPFS_API_CREATE_FOLDER, kpfs_api_root_get(), (char *)path);
	KPFS_SAFE_FREE(response);
	return KPFS_RET_OK;
}

char *kpfs_api_upload_locate()
{
	char url[KPFS_MAX_BUF] = { 0 };

	snprintf(url, sizeof(url), "%s", KPFS_API_UPLOAD_LOCATE);

	return kpfs_api_common_request(url);
}

char *kpfs_api_upload_file(char *dest_fullpath, char *src_fullpath)
{
	char *t_key = kpfs_oauth_token_get();
	char *t_secret = kpfs_oauth_token_secret_get();
	char *postarg = NULL;
	char *req_url = NULL;
	char *reply = NULL;
	char *ret = NULL;
	char *dest_fullpath_escape = NULL;
	int malloc_more_room = 100;
	char *url_with_path = NULL;
	int len = 0;
	char *url = kpfs_util_upload_locate_get();
	char *root = NULL;
	char post_url[KPFS_MAX_BUF] = { 0 };

	if (NULL == dest_fullpath || NULL == src_fullpath)
		return ret;

	KPFS_FILE_LOG("request url: %s ...\n", url);
	root = kpfs_api_root_get();

	dest_fullpath_escape = oauth_url_escape(dest_fullpath);
	if (NULL == dest_fullpath_escape)
		goto error_out;

	len = strlen(url) + strlen(KPFS_API_UPLOAD_FILE) + strlen(root) + strlen(dest_fullpath_escape) + malloc_more_room;
	url_with_path = calloc(len, 1);
	if (NULL == url_with_path)
		goto error_out;

	snprintf(url_with_path, len, "%s%s?root=%s&path=%s&overwrite=true", url, KPFS_API_UPLOAD_FILE, root, dest_fullpath_escape);

	KPFS_FILE_LOG("url_with_path: %s, ...\n", url_with_path);

	req_url =
	    oauth_sign_url2(url_with_path, &postarg, OA_HMAC, NULL, (const char *)kpfs_conf_get_consumer_key(), (const char *)kpfs_conf_get_consumer_secret(),
			    t_key, t_secret);
	KPFS_SAFE_FREE(url_with_path);
	if (!req_url)
		goto error_out;

	snprintf(post_url, sizeof(post_url), "%s?%s", req_url, postarg);
	KPFS_FILE_LOG("post_url: %s.\n", post_url);

	reply = calloc(KPFS_MAX_BUF, 1);
	if (NULL == reply)
		goto error_out;

	kpfs_curl_upload(post_url, src_fullpath, reply);

	KPFS_FILE_LOG("upload reply: %s\n", reply);
	ret = reply;

error_out:
	KPFS_SAFE_FREE(dest_fullpath_escape);
	KPFS_SAFE_FREE(req_url);

	return ret;
}
