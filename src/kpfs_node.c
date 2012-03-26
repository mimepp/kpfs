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
#include <sys/types.h>
#include <sys/stat.h>
#include <oauth.h>
#include <pthread.h>

#include "kpfs.h"
#include "kpfs_node.h"

kpfs_node *g_kpfs_node_root = NULL;

void kpfs_node_free(gpointer p)
{
	kpfs_node *node = (kpfs_node *) p;
	KPFS_SAFE_FREE(node->id);
	KPFS_SAFE_FREE(node->name);
	KPFS_SAFE_FREE(node->revision);
	pthread_mutex_destroy(&(node->mutex));
	KPFS_SAFE_FREE(node);
}

kpfs_node *kpfs_node_root_get()
{
	return g_kpfs_node_root;
}

kpfs_node *kpfs_node_root_create(char *id, char *name, off_t size)
{
	kpfs_node *root = NULL;

	if (NULL != g_kpfs_node_root)
		return g_kpfs_node_root;

	root = calloc(sizeof(kpfs_node), 1);
	if (NULL == root)
		return NULL;

	root->fullpath = strdup("/");
	root->id = id;
	root->name = name;
	root->revision = strdup("1");
	root->type = KPFS_NODE_TYPE_FOLDER;
	root->is_deleted = 0;
	root->st.st_mode = S_IFDIR | 0755;
	root->st.st_nlink = 2;
	root->st.st_size = size;
	pthread_mutex_init(&(root->mutex), NULL);
	root->sub_nodes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, kpfs_node_free);
	g_kpfs_node_root = root;

	return g_kpfs_node_root;
}

kpfs_node *kpfs_node_get_by_path(kpfs_node * node, const char *path)
{
	kpfs_node *ret = NULL;
	GHashTableIter iter;

	if (NULL == path || NULL == node)
		return NULL;

	if (1 == strlen(path) && path[0] == '/')
		return kpfs_node_root_get();

	ret = g_hash_table_lookup(node->sub_nodes, path);
	if (NULL == ret) {
		char *key = NULL;
		kpfs_node *value = NULL;

		KPFS_LOG("[%s:%d] could not find %s in node->sub_nodes.\n", __FUNCTION__, __LINE__, path);
		g_hash_table_iter_init(&iter, node->sub_nodes);
		while (g_hash_table_iter_next(&iter, (gpointer *) & key, (gpointer *) & value)) {
			KPFS_LOG("[%s:%d] iter, key: %s, path: %s\n", __FUNCTION__, __LINE__, key, path);
			ret = kpfs_node_get_by_path(value, path);
			if (ret)
				return ret;
		}
	} else {
		return ret;
	}
	return ret;
}

void kpfs_node_dump(kpfs_node * node)
{
	if (NULL == node) {
		KPFS_FILE_LOG("[%s:%d] node is NULL.\n", __FUNCTION__, __LINE__);
		return;
	}

	KPFS_FILE_LOG("node date:\n");
	KPFS_FILE_LOG("\tfullpath: %s\n", node->fullpath);
	KPFS_FILE_LOG("\tid: %s\n", node->id);
	KPFS_FILE_LOG("\tname: %s\n", node->name);
	KPFS_FILE_LOG("\trevision: %s\n", node->revision);
	KPFS_FILE_LOG("\ttype: %d (0:file, 1: folder)\n", node->type);
	KPFS_FILE_LOG("\tis_deleted: %d\n", node->is_deleted);
	KPFS_FILE_LOG("\tst.st_size: %lld\n", node->st.st_size);
	KPFS_FILE_LOG("\tst.st_ctime: %lu\n", node->st.st_ctime);
	KPFS_FILE_LOG("\tst.st_mtime: %lu\n", node->st.st_mtime);
}
