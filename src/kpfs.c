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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <fuse.h>
#include <fuse_opt.h>
#include <pthread.h>

#define __STRICT_ANSI__
#include <json/json.h>

#include "kpfs.h"
#include "kpfs_node.h"
#include "kpfs_conf.h"
#include "kpfs_oauth.h"
#include "kpfs_api.h"

static int kpfs_get_root_path()
{
	char *response = NULL;
	json_object *jobj = NULL;
	off_t size = 0;
	int len = 512;
	char *user_name = NULL;
	char *user_id = NULL;

	response = (char *)kpfs_api_account_info();
	KPFS_FILE_LOG("access [/] %s:\n", response);

	jobj = json_tokener_parse(response);
	if (NULL == jobj || is_error(jobj)) {
		KPFS_FILE_LOG("%s:%d, json_tokener_parse return error.\n", __FUNCTION__, __LINE__);
		KPFS_SAFE_FREE(response);
		return -1;
	}
	json_object_object_foreach(jobj, key, val) {
		if (!strcmp(key, KPFS_ID_QUOTA_USED)) {
			size = (off_t) json_object_get_double(val);
		} else if (!strcmp(key, KPFS_ID_USER_ID)) {
			if (json_type_int == json_object_get_type(val)) {
				user_id = calloc(len, 1);
				snprintf(user_id, len, "%d", json_object_get_int(val));
			}
		} else if (!strcmp(key, KPFS_ID_USER_NAME)) {
			if (json_type_string == json_object_get_type(val)) {
				user_name = calloc(len, 1);
				snprintf(user_name, len, "%s", json_object_get_string(val));
			}
		}
	}
	json_object_put(jobj);
	KPFS_SAFE_FREE(response);

	if (NULL == kpfs_node_root_create(user_id, user_name, size))
		return -1;

	return 0;
}

static time_t kpfs_str2time(char *str)
{
	struct timeval tv;
	time_t time;
	struct tm atm;
	int year = 0;
	int month = 0;
	int day = 0;
	int hour = 0;
	int min = 0;
	int sec = 0;

	gettimeofday(&tv, NULL);
	time = tv.tv_sec;

	if (NULL == str)
		return time;

	/* 2011-09-19 18:13:13 */
	sscanf(str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec);

	atm.tm_year = year - 1900;
	atm.tm_mon = month - 1;
	atm.tm_mday = day;
	atm.tm_hour = hour;
	atm.tm_min = min;
	atm.tm_sec = sec;

	time = mktime(&atm);

	return time;
}

static kpfs_ret kpfs_parse_dir(kpfs_node * parent_node, const char *path)
{
	char *response = NULL;
	json_object *jobj;
	json_object *tmp_jobj;
	kpfs_node *node = NULL;
	int len = 0;
	char *parent_path = NULL;
	kpfs_ret ret = KPFS_RET_FAIL;

	if (NULL == parent_node)
		return KPFS_RET_FAIL;

	response = (char *)kpfs_api_metadata(path);
	KPFS_LOG("response %s:\n", response);

	jobj = json_tokener_parse(response);
	if (NULL == jobj || is_error(jobj)) {
		KPFS_FILE_LOG("%s:%d, json_tokener_parse return error.\n", __FUNCTION__, __LINE__);
		KPFS_SAFE_FREE(response);
		return KPFS_RET_FAIL;
	}

	json_object_object_foreach(jobj, key, val) {
		if (!strcmp(key, KPFS_ID_MSG)) {
			if (json_type_string == json_object_get_type(val)) {
				if (0 == strcmp(KPFS_MSG_STR_REUSED_NONCE, json_object_get_string(val))) {
					KPFS_FILE_LOG("%s:%d, receive reused nonce.\n", __FUNCTION__, __LINE__);
					goto error_out;
				}
			}
		} else if (!strcmp(key, KPFS_ID_PATH)) {
		} else if (!strcmp(key, KPFS_ID_ROOT)) {
			if (json_type_string == json_object_get_type(val)) {
			}
		} else if (!strcmp(key, KPFS_ID_HASH)) {
		} else if (!strcmp(key, KPFS_ID_FILES)) {
			if (json_type_array == json_object_get_type(val)) {
				int j = 0, array_len = 0;

				array_len = json_object_array_length(val);
				for (j = 0; j < array_len; j++) {
					tmp_jobj = json_object_array_get_idx(val, j);
					if (tmp_jobj && !is_error(tmp_jobj)) {
						node = calloc(sizeof(kpfs_node), 1);
						if (NULL == node) {
							KPFS_FILE_LOG("%s:%d, fail to calloc node.\n", __FUNCTION__, __LINE__);
							goto error_out;
						}
						errno = pthread_mutex_init(&(node->mutex), NULL);
						if (errno) {
							KPFS_FILE_LOG("%s:%d, pthread_mutex_init fail.\n", __FUNCTION__, __LINE__);
							goto error_out;
						}
						node->sub_nodes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, kpfs_node_free);
						len = strlen(path) + 1;
						parent_path = calloc(len, 1);
						snprintf(parent_path, len, "%s", path);

						KPFS_FILE_LOG("\n", __FUNCTION__, __LINE__);
						json_object_object_foreach(tmp_jobj, key2, val2) {
							if (!strcmp(key2, KPFS_ID_IS_DELETED)) {
								if (json_type_boolean == json_object_get_type(val2)) {
									node->is_deleted = json_object_get_boolean(val2) == TRUE ? 1 : 0;
									KPFS_FILE_LOG("%s:%d, is_deleted: %d.\n", __FUNCTION__, __LINE__, node->is_deleted);
								}
							} else if (!strcmp(key2, KPFS_ID_NAME)) {
								if (json_type_string == json_object_get_type(val2)) {
									node->name = strdup(json_object_get_string(val2));
									KPFS_FILE_LOG("%s:%d, name: %s.\n", __FUNCTION__, __LINE__, node->name);
								}
							} else if (!strcmp(key2, KPFS_ID_REV)) {
								if (json_type_string == json_object_get_type(val2)) {
									node->revision = strdup(json_object_get_string(val2));
									KPFS_FILE_LOG("%s:%d, revision: %s.\n", __FUNCTION__, __LINE__,
										      json_object_get_string(val2));
								}
							} else if (!strcmp(key2, KPFS_ID_CREATE_TIME)) {
								if (json_type_string == json_object_get_type(val2)) {
									char str[128] = { 0 };
									snprintf(str, sizeof(str), "%s", json_object_get_string(val2));
									node->st.st_ctime = kpfs_str2time(str);
									KPFS_FILE_LOG("%s:%d, ctime: %d.\n", __FUNCTION__, __LINE__, node->st.st_ctime);
								}
							} else if (!strcmp(key2, KPFS_ID_MODIFY_TIME)) {
								if (json_type_string == json_object_get_type(val2)) {
									char str[128] = { 0 };
									snprintf(str, sizeof(str), "%s", json_object_get_string(val2));
									node->st.st_mtime = kpfs_str2time(str);
									KPFS_FILE_LOG("%s:%d, mtime: %d.\n", __FUNCTION__, __LINE__, node->st.st_mtime);
								}
							} else if (!strcmp(key2, KPFS_ID_SIZE)) {
								if (json_type_int == json_object_get_type(val2)) {
									node->st.st_size = json_object_get_int(val2);
									KPFS_FILE_LOG("%s:%d, size: %d.\n", __FUNCTION__, __LINE__, node->st.st_size);
								}
							} else if (!strcmp(key2, KPFS_ID_TYPE)) {
								if (json_type_string == json_object_get_type(val2)) {
									if (0 == strcasecmp(KPFS_NODE_TYPE_STR_FILE, json_object_get_string(val2))) {
										node->type = KPFS_NODE_TYPE_FILE;
									} else {
										node->type = KPFS_NODE_TYPE_FOLDER;
									}
									KPFS_FILE_LOG("%s:%d, type: %d.\n", __FUNCTION__, __LINE__, node->type);
								}
							} else if (!strcmp(key2, KPFS_ID_FILE_ID)) {
								if (json_type_string == json_object_get_type(val2)) {
									node->id = strdup(json_object_get_string(val2));
									KPFS_FILE_LOG("%s:%d, id: %s.\n", __FUNCTION__, __LINE__, node->id);
								}
							}
						}
						if (node->name) {
							if (1 == strlen(parent_path) && parent_path[0] == '/') {
								len = strlen(parent_path) + strlen(node->name) + 1;
								node->fullpath = calloc(len, 1);
								snprintf(node->fullpath, len, "%s%s", parent_path, node->name);
							} else {
								len = strlen(parent_path) + strlen("/") + strlen(node->name) + 1;
								node->fullpath = calloc(len, 1);
								snprintf(node->fullpath, len, "%s/%s", parent_path, node->name);
							}
							KPFS_LOG("%s:%d, fullpath: %s.\n", __FUNCTION__, __LINE__, node->fullpath);
							if (KPFS_NODE_TYPE_FOLDER == node->type) {
								kpfs_parse_dir(node, node->fullpath);
								node->st.st_mode = S_IFDIR | 0755;
								node->st.st_nlink = 2;
								if (0 == node->st.st_size)
									node->st.st_size = getpagesize();
							} else {
								node->st.st_mode = S_IFREG | 0666;
								node->st.st_nlink = 1;
							}

							KPFS_NODE_LOCK(parent_node);
							g_hash_table_insert(parent_node->sub_nodes, node->fullpath, node);
							KPFS_NODE_UNLOCK(parent_node);
						}
					}
				}
			}
		}
	}
	ret = KPFS_RET_OK;
error_out:
	json_object_put(jobj);

	KPFS_SAFE_FREE(response);
	return ret;
}

static int kpfs_getattr(const char *path, struct stat *stbuf)
{
	kpfs_node *node = NULL;

	if (!path || !stbuf)
		return -1;

	memset(stbuf, 0, sizeof(*stbuf));

	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	KPFS_FILE_LOG("path: %s\n", path);

	if (0 == strcmp(path, "/.Trash") || 0 == strcmp(path, "/.Trash-1000")) {
		KPFS_FILE_LOG("we will not handle %s\n", path);
		return -1;
	}
	node = kpfs_node_get_by_path((kpfs_node *) kpfs_node_root_get(), path);
	if (NULL == node) {
		KPFS_FILE_LOG("%s:%d, could not find: %s\n", __FUNCTION__, __LINE__, path);
		return -1;
	}
	kpfs_node_dump(node);
	memcpy(stbuf, &(node->st), sizeof(node->st));
	return 0;
}

static int kpfs_access(const char *path, int mask)
{
	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	KPFS_FILE_LOG("[%s:%d] path: %s, mask: %d.\n", __FUNCTION__, __LINE__, path, mask);
	return 0;
}

static int kpfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	kpfs_node *node = NULL;

	(void)offset;
	(void)fi;

	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	node = kpfs_node_get_by_path((kpfs_node *) kpfs_node_root_get(), path);
	if (node) {
		GHashTableIter iter;
		char *key = NULL;
		kpfs_node *value = NULL;

		if (1 != strlen(path)) {
			//kpfs_parse_dir(node, node->fullpath);
		}

		g_hash_table_iter_init(&iter, node->sub_nodes);
		while (g_hash_table_iter_next(&iter, (gpointer *) & key, (gpointer *) & value)) {
			KPFS_FILE_LOG("%s:%d, path: %s, filler: %s\n", __FUNCTION__, __LINE__, path, value->name);
			filler(buf, value->name, NULL, 0);
		}
	} else {
		return -1;
	}
	return 0;
}

static int kpfs_read(const char *path, char *rbuf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	KPFS_FILE_LOG("[%s:%d] path: %s, rbuf:%s, size: %lu, offset: %lu, file info: %p\n", __FUNCTION__, __LINE__, path, rbuf, size, offset, fi);
	return 0;
}

static int kpfs_write(const char *path, const char *wbuf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	KPFS_FILE_LOG("[%s:%d] path: %s, wbuf:%s, size: %lu, offset: %lu, file info: %p\n", __FUNCTION__, __LINE__, path, wbuf, size, offset, fi);
	return 0;
}

static int kpfs_statfs(const char *path, struct statvfs *buf)
{
	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	KPFS_FILE_LOG("[%s:%d] path: %s, statvfs: %p.\n", __FUNCTION__, __LINE__, path, buf);
	return 0;
}

static int kpfs_mkdir(const char *path, mode_t mode)
{
	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	KPFS_FILE_LOG("[%s:%d] path: %s, mode: %d.\n", __FUNCTION__, __LINE__, path, mode);
	return 0;
}

static int kpfs_rmdir(const char *path)
{
	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	KPFS_FILE_LOG("[%s:%d] path: %s.\n", __FUNCTION__, __LINE__, path);
	return 0;
}

static int kpfs_release(const char *path, struct fuse_file_info *fi)
{
	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	KPFS_FILE_LOG("[%s:%d] path: %s, file info: %p.\n", __FUNCTION__, __LINE__, path, fi);
	return 0;
}

static int kpfs_rename(const char *from, const char *to)
{
	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	KPFS_FILE_LOG("[%s:%d] from: %s, to: %s.\n", __FUNCTION__, __LINE__, from, to);
	return 0;
}

static int kpfs_truncate(const char *path, off_t offset)
{
	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	KPFS_FILE_LOG("[%s:%d] path: %s, offset: %d.\n", __FUNCTION__, __LINE__, path, offset);
	return 0;
}

static int kpfs_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];

	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(path, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int kpfs_open(const char *path, struct fuse_file_info *fi)
{
	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	KPFS_FILE_LOG("[%s:%d] path: %s, file info: %p.\n", __FUNCTION__, __LINE__, path, fi);
	return 0;
}

static int kpfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	KPFS_FILE_LOG("[%s:%d] path: %s, mode: %d, file info: %p.\n", __FUNCTION__, __LINE__, path, mode, fi);
	return 0;
}

static int kpfs_unlink(const char *path)
{
	KPFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	KPFS_FILE_LOG("[%s:%d] path: %s.\n", __FUNCTION__, __LINE__, path);
	return 0;
}

static struct fuse_operations kpfs_oper = {
	.getattr = kpfs_getattr,
	.access = kpfs_access,
	.readdir = kpfs_readdir,
	.mkdir = kpfs_mkdir,
	.unlink = kpfs_unlink,
	.rmdir = kpfs_rmdir,
	.release = kpfs_release,
	.rename = kpfs_rename,
	.truncate = kpfs_truncate,
	.utimens = kpfs_utimens,
	.open = kpfs_open,
	.create = kpfs_create,
	.read = kpfs_read,
	.write = kpfs_write,
	.statfs = kpfs_statfs,
};

static void kpfs_usage(char *argv0)
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
	int fuse_ret = 0;
	struct stat st;
	int fuse_argc = 2;
	char *fuse_argv[2] = { 0 };

	if (argc < 3) {
		kpfs_usage(argv[0]);
		return -1;
	}
	if (0 == strcmp("-c", argv[1])) {
		if (argv[2] == '\0') {
			kpfs_usage(argv[0]);
			return -1;
		}
		if (0 != stat(argv[2], &st)) {
			printf("config file of kpfs (%s) is not existed.\n", argv[2]);
			kpfs_usage(argv[0]);
			return -1;
		}
		if (KPFS_RET_OK == kpfs_conf_load(argv[2])) {
			kpfs_conf_dump();
			fuse_argv[0] = calloc(KPFS_MAX_PATH, 1);
			if (NULL == fuse_argv[0]) {
				printf("%s:%d, calloc fail.\n", __FUNCTION__, __LINE__);
				return -1;
			}
			fuse_argv[1] = calloc(KPFS_MAX_PATH, 1);
			if (NULL == fuse_argv[1]) {
				printf("%s:%d, calloc fail.\n", __FUNCTION__, __LINE__);
				return -1;
			}
			snprintf(fuse_argv[0], KPFS_MAX_PATH, "%s", argv[0]);
			snprintf(fuse_argv[1], KPFS_MAX_PATH, "%s", kpfs_conf_get_mount_point());
		}
	} else {
		kpfs_usage(argv[0]);
		return -1;
	}

	kpfs_oauth_init();

	ret = kpfs_oauth_load();
	if (KPFS_RET_OK != ret) {
		printf("This is the first time you use kpfs.\n");
		ret = kpfs_oauth_request_token();
		if (KPFS_RET_OK != ret) {
			return -1;
		}
		ret = kpfs_oauth_authorize();
		if (KPFS_RET_OK != ret) {
			return -1;
		}
		ret = kpfs_oauth_access_token();
		if (KPFS_RET_OK != ret) {
			KPFS_LOG("Fail to get oauth token for kpfs.\n");
			return -1;
		}
	}
	kpfs_get_root_path();
	kpfs_parse_dir((kpfs_node *) kpfs_node_root_get(), "/");

	fuse_ret = fuse_main(fuse_argc, fuse_argv, &kpfs_oper, NULL);

	return fuse_ret;
}
