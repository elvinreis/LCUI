/*
 * css_library.c -- CSS library operation module.
 *
 * Copyright (c) 2018, Liu chao <lc-soft@live.cn> All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of LCUI nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/css/value.h"
#include "../include/css/style_value.h"
#include "../include/css/selector.h"
#include "../include/css/library.h"

#define LEN(A) sizeof(A) / sizeof(*A)

/** dict_t<string, css_style_link_group_t*> */
typedef dict_t css_style_group_t;

/** 样式链接记录组 */
typedef struct css_style_link_group_t {
	dict_t *links;              /**< 样式链接表 */
	char *name;                 /**< 选择器名称 */
	css_selector_node_t *snode; /**< 选择器结点 */
} css_style_link_group_t;

/** 样式结点记录 */
typedef struct css_style_rule_t {
	int rank;                /**< 权值，决定优先级 */
	int batch_num;           /**< 批次号 */
	char *space;             /**< 所属的空间 */
	char *selector;          /**< 选择器 */
	css_style_props_t *list; /**< 样式表 */
	list_node_t node;        /**< 在链表中的结点 */
} css_style_rule_t;

/** 样式链接记录 */
typedef struct css_style_link_t {
	char *selector;                /**< 选择器 */
	css_style_link_group_t *group; /**< 所属组 */
	list_t styles;                 /**< 作用于当前选择器的样式 */
	dict_t *parents;               /**< 父级节点 */
} css_style_link_t;

static struct css_library_module_t {
	/**
	 * 样式组列表
	 * list_t<css_style_group_t*>
	 */
	list_t groups;

	/** 字符串池 */
	strpool_t *strpool;

	/**
	 * 样式表缓存，以选择器的 hash 值索引
	 * dict_t<css_selector_hash_t, css_style_props_t*>
	 */
	dict_t *cache;

} css_library;

/* clang-format off */

static uint64_t ikey_dict_hash(const void *key)
{
	return (*(unsigned int *)key);
}

static int ikey_dict_key_compare(void *privdata, const void *key1,
				 const void *key2)
{
	return *(unsigned int *)key1 == *(unsigned int *)key2;
}

static void ikey_dict_key_destructor(void *privdata, void *key)
{
	free(key);
}

static void *ikey_dict_key_dup(void *privdata, const void *key)
{
	unsigned int *newkey = malloc(sizeof(unsigned int));
	*newkey = *(unsigned int *)key;
	return newkey;
}

static void css_style_cache_destructor(void *privdata, void *val)
{
	css_style_declaration_destroy(val);
}

LCUI_BOOL css_selector_node_match(css_selector_node_t *sn1, css_selector_node_t *sn2)
{
	int i, j;
	if (sn2->id) {
		if (!sn1->id || strcmp(sn1->id, sn2->id) != 0) {
			return FALSE;
		}
	}
	if (sn2->type && strcmp(sn2->type, "*") != 0) {
		if (!sn1->type || strcmp(sn1->type, sn2->type) != 0) {
			return FALSE;
		}
	}
	if (sn2->classes) {
		if (!sn1->classes) {
			return FALSE;
		}
		for (i = 0; sn2->classes[i]; ++i) {
			for (j = 0; sn1->classes[j]; ++j) {
				if (strcmp(sn2->classes[i], sn1->classes[i]) ==
				    0) {
					j = -1;
					break;
				}
			}
			if (j != -1) {
				return FALSE;
			}
		}
	}
	if (sn2->status) {
		if (!sn1->status) {
			return FALSE;
		}
		for (i = 0; sn2->status[i]; ++i) {
			for (j = 0; sn1->status[j]; ++j) {
				if (strcmp(sn2->status[i], sn1->status[i]) ==
				    0) {
					j = -1;
					break;
				}
			}
			if (j != -1) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

css_style_props_t *css_style_properties_create(void)
{
	css_style_props_t *list;

	list = malloc(sizeof(css_style_props_t));
	list_create(list);
	return list;
}

static void css_style_property_destroy(css_style_property_t *node)
{
	css_style_value_destroy(&node->style);
	free(node);
}

void css_style_properties_destroy(css_style_props_t *list)
{
	list_destroy_without_node(list, (list_item_destructor_t)css_style_property_destroy);
	free(list);
}

css_style_decl_t *css_style_declaration_create(void)
{
	css_style_decl_t *ss;

	ss = calloc(sizeof(css_style_decl_t), 1);
	if (!ss) {
		return ss;
	}
	ss->length = css_get_property_count();
	ss->list = calloc(sizeof(css_style_value_t), ss->length + 1);
	return ss;
}

void css_style_declaration_clear(css_style_decl_t *ss)
{
	unsigned i;

	for (i = 0; i < ss->length; ++i) {
		css_style_value_destroy(&ss->list[i]);
	}
}

void css_style_declaration_destroy(css_style_decl_t *ss)
{
	css_style_declaration_clear(ss);
	free(ss->list);
	free(ss);
}

css_style_property_t *css_style_properties_find(css_style_props_t *list, int key)
{
	list_node_t *node;
	css_style_property_t *snode;

	for (list_each(node, list)) {
		snode = node->data;
		if (snode->key == key) {
			return snode;
		}
	}
	return NULL;
}

int css_style_properties_remove(css_style_props_t *list, int key)
{
	list_node_t *node;
	css_style_property_t *snode;

	for (list_each(node, list)) {
		snode = node->data;
		if (snode->key == key) {
			list_unlink(list, node);
			css_style_value_destroy(&snode->style);
			free(snode);
			return 0;
		}
	}
	return -1;
}

css_style_property_t *css_style_properties_add(css_style_props_t *list, int key)
{
	css_style_property_t *node;

	node = malloc(sizeof(css_style_property_t));
	node->key = key;
	node->style.type = CSS_NO_VALUE;
	node->node.data = node;
	list_append_node(list, &node->node);
	return node;
}

static unsigned css_style_properties_merge(css_style_props_t *list, const css_style_decl_t *style)
{
	unsigned i, count;
	css_style_property_t *node;

	for (count = 0, i = 0; i < style->length; ++i) {
		if (!style->list[i].type > CSS_INVALID_VALUE) {
			continue;
		}
		node = css_style_properties_add(list, (int)i);
		css_style_value_merge(&node->style, &style->list[i]);
		count += 1;
	}
	return count;
}

int css_style_declaration_expand(css_style_decl_t *style, unsigned length)
{
	size_t i;
	css_style_value_t *s;

	if (length > style->length) {
		s = realloc(style->list, sizeof(css_style_value_t) * length);
		if (!s) {
			return -1;
		}
		for (i = style->length; i < length; ++i) {
			s[i].type = CSS_NO_VALUE;
		}
		style->list = s;
		style->length = length;
	}
	return 0;
}

int css_style_declaration_merge(css_style_decl_t *dest, const css_style_decl_t *src)
{
	size_t i;

	if (css_style_declaration_expand(dest, src->length) != 0) {
		return -1;
	}
	for (i = 0; i < src->length; ++i) {
		if (src->list[i].type > CSS_INVALID_VALUE && !dest->list[i].type > CSS_INVALID_VALUE) {
			css_style_value_merge(&dest->list[i], &src->list[i]);
		}
	}
	return 0;
}

int css_style_declaration_merge_properties(css_style_decl_t *ss, css_style_props_t *list)
{
	css_style_property_t *snode;
	list_node_t *node;
	size_t count = 0;

	for (list_each(node, list)) {
		snode = node->data;
		if (css_style_declaration_expand(ss, snode->key + 1) != 0) {
			return -1;
		}
		if (!ss->list[snode->key].type > CSS_INVALID_VALUE && snode->style.type > CSS_INVALID_VALUE) {
			css_style_value_merge(&ss->list[snode->key], &snode->style);
			++count;
		}
	}
	return (int)count;
}

int css_style_declaration_replace(css_style_decl_t *dest, const css_style_decl_t *src)
{
	size_t i;
	size_t count;

	if (css_style_declaration_expand(dest, src->length) != 0) {
		return -1;
	}
	for (count = 0, i = 0; i < src->length; ++i) {
		if (!src->list[i].type > CSS_INVALID_VALUE) {
			continue;
		}
		css_style_value_destroy(&dest->list[i]);
		css_style_value_merge(&dest->list[i], &src->list[i]);
		++count;
	}
	return (int)count;
}

static void css_style_rule_destroy(css_style_rule_t *node)
{
	if (node->space) {
		strpool_free_str(node->space);
		node->space = NULL;
	}
	if (node->selector) {
		free(node->selector);
		node->selector = NULL;
	}
	css_style_properties_destroy(node->list);
	node->list = NULL;
	free(node);
}

static css_style_link_t *css_style_link_create(void)
{
	css_style_link_t *link = calloc(sizeof(css_style_link_t), 1);
	static dict_type_t t;

	dict_init_string_copy_key_type(&t);
	link->group = NULL;
	list_create(&link->styles);
	link->parents = dict_create(&t, NULL);
	return link;
}

static void css_style_link_destroy(css_style_link_t *link)
{
	dict_destroy(link->parents);
	list_destroy_without_node(&link->styles, (list_item_destructor_t)css_style_rule_destroy);
	free(link->selector);
	link->selector = NULL;
	link->parents = NULL;
	link->group = NULL;
	free(link);
}

static void css_style_link_destructor(void *privdata, void *data)
{
	css_style_link_destroy(data);
}

static css_style_link_group_t *css_style_link_group_create(css_selector_node_t *snode)
{
	static dict_type_t dt = { 0 };
	css_style_link_group_t *group;

	dict_init_string_copy_key_type(&dt);
	dt.val_destructor = css_style_link_destructor;
	group = calloc(sizeof(css_style_link_group_t), 1);
	group->snode = calloc(sizeof(css_selector_node_t), 1);
	group->links = dict_create(&dt, NULL);
	group->snode = css_selector_node_duplicate(snode);
	group->name = group->snode->fullname;
	return group;
}

static void css_style_link_group_destroy(css_style_link_group_t *group)
{
	dict_type_t *dtype;
	dtype = group->links->priv_data;
	css_selector_node_destroy(group->snode);
	dict_destroy(group->links);
	free(dtype);
	free(group);
}

static void css_style_link_group_destructor(void *privdata, void *data)
{
	css_style_link_group_destroy(data);
}

static css_style_group_t *css_style_group_create(void)
{
	static dict_type_t type = { 0 };

	dict_init_string_copy_key_type(&type);
	type.val_destructor = css_style_link_group_destructor;
	return dict_create(&type, NULL);
}

/** 根据选择器，选中匹配的样式表 */
static css_style_props_t *css_select_style_properties(css_selector_t *selector,
					   const char *space)
{
	int i, right;
	css_style_link_t *link;
	css_style_rule_t *snode;
	css_style_link_group_t *slg;
	css_selector_node_t *sn;
	css_style_group_t *group;
	dict_t *parents;

	char buf[CSS_SELECTOR_MAX_LEN];
	char fullname[CSS_SELECTOR_MAX_LEN];

	link = NULL;
	parents = NULL;
	for (i = 0, right = selector->length - 1; right >= 0; --right, ++i) {
		group = list_get(&css_library.groups, i);
		if (!group) {
			group = css_style_group_create();
			list_append(&css_library.groups, group);
		}
		sn = selector->nodes[right];
		slg = dict_fetch_value(group, sn->fullname);
		if (!slg) {
			slg = css_style_link_group_create(sn);
			dict_add(group, sn->fullname, slg);
		}
		if (i == 0) {
			strcpy(fullname, "*");
		} else {
			strcpy(fullname, buf);
		}
		link = dict_fetch_value(slg->links, fullname);
		if (!link) {
			link = css_style_link_create();
			link->group = slg;
			link->selector = strdup2(fullname);
			dict_add(slg->links, fullname, link);
		}
		if (i == 0) {
			strcpy(buf, sn->fullname);
			strcpy(fullname, buf);
		} else {
			strcpy(fullname, buf);
			sprintf(buf, "%s %s", sn->fullname, fullname);
		}
		/* 如果有上一级的父链接记录，则将当前链接添加进去 */
		if (parents) {
			if (!dict_fetch_value(parents, sn->fullname)) {
				dict_add(parents, sn->fullname, link);
			}
		}
		parents = link->parents;
	}
	if (!link) {
		return NULL;
	}
	snode = calloc(sizeof(css_style_rule_t), 1);
	if (space) {
		snode->space = strpool_alloc_str(css_library.strpool, space);
		strcpy(snode->space, space);
	} else {
		snode->space = NULL;
	}
	snode->node.data = snode;
	snode->list = css_style_properties_create();
	snode->rank = selector->rank;
	snode->selector = strdup2(fullname);
	snode->batch_num = selector->batch_num;
	list_append_node(&link->styles, &snode->node);
	return snode->list;
}

int css_add_style_sheet(css_selector_t *selector, css_style_decl_t *style,
		       const char *space)
{
	css_style_props_t *list;
	dict_empty(css_library.cache, NULL);
	list = css_select_style_properties(selector, space);
	if (list) {
		css_style_properties_merge(list, style);
	}
	return 0;
}

static size_t StyleLink_GetStyleSheets(css_style_link_t *link, list_t *outlist)
{
	size_t i;
	LCUI_BOOL found;
	css_style_rule_t *snode, *out_snode;
	list_node_t *node, *out_node;

	if (!outlist) {
		return link->styles.length;
	}
	for (list_each(node, &link->styles)) {
		i = 0;
		found = FALSE;
		snode = node->data;
		for (list_each(out_node, outlist)) {
			out_snode = out_node->data;
			if (snode->rank > out_snode->rank) {
				found = TRUE;
				break;
			}
			if (snode->rank != out_snode->rank) {
				i += 1;
				continue;
			}
			if (snode->batch_num > out_snode->batch_num) {
				found = TRUE;
				break;
			}
			i += 1;
		}
		if (found) {
			list_insert(outlist, i, snode);
		} else {
			list_append(outlist, snode);
		}
	}
	return link->styles.length;
}

static size_t css_query_selector_from_link(css_style_link_t *link, css_selector_t *s,
					  int i, list_t *list)
{
	size_t count = 0;
	css_style_link_t *parent;
	list_t names;
	list_node_t *node;
	css_selector_node_t *sn;

	list_create(&names);
	count += StyleLink_GetStyleSheets(link, list);
	while (--i >= 0) {
		sn = s->nodes[i];
		css_selector_node_get_name_list(sn, &names);
		for (list_each(node, &names)) {
			parent = dict_fetch_value(link->parents, node->data);
			if (!parent) {
				continue;
			}
			count +=
			    css_query_selector_from_link(parent, s, i, list);
		}
		list_destroy(&names, free);
	}
	return count;
}

int css_query_selector_from_group(int group, const char *name, css_selector_t *s,
				 list_t *list)
{
	int i;
	size_t count;
	dict_t *groups;
	css_style_link_group_t *slg;
	list_node_t *node;
	list_t names;

	groups = list_get(&css_library.groups, group);
	if (!groups || s->length < 1) {
		return 0;
	}
	count = 0;
	i = s->length - 1;
	list_create(&names);
	if (name) {
		list_append(&names, strdup2(name));
	} else {
		css_selector_node_get_name_list(s->nodes[i], &names);
		list_append(&names, strdup2("*"));
	}
	for (list_each(node, &names)) {
		dict_entry_t *entry;
		dict_iterator_t *iter;
		char *name = node->data;
		slg = dict_fetch_value(groups, name);
		if (!slg) {
			continue;
		}
		iter = dict_get_iterator(slg->links);
		while ((entry = dict_next(iter))) {
			css_style_link_t *link = dict_get_val(entry);
			count += css_query_selector_from_link(link, s, i, list);
		}
		dict_destroy_iterator(iter);
	}
	list_destroy(&names, free);
	return (int)count;
}

static void css_print_property_name(int key)
{
	const css_property_definition_t *prop;

	prop = css_get_property_by_key(key);
	if (prop) {
		logger_debug("\t%s", prop->name);
	} else {
		logger_debug("\t<unknown property %d>", key);
	}
	logger_debug("%s: ", key > STYLE_KEY_TOTAL ? " (+)" : "");
}

void css_style_properties_print(css_style_props_t *list)
{
	list_node_t *node;
	css_style_property_t *snode;
	char str[256] = { 0 };

	for (list_each(node, list)) {
		snode = node->data;
		if (snode->style.type != CSS_NO_VALUE) {
			css_print_property_name(snode->key);
			css_style_value_to_string(&snode->style, str, 255);
			printf("%s;\n", str);
			str[0] = 0;
		}
	}
}

void css_style_declaration_print(css_style_decl_t *ss)
{
	unsigned key;
	css_style_value_t *s;
	char str[256] = { 0 };

	for (key = 0; key < ss->length; ++key) {
		s = &ss->list[key];
		if (s->type != CSS_NO_VALUE) {
			css_print_property_name(key);
			css_style_value_to_string(s, str, 255);
			printf("%s;\n", str);
			str[0] = 0;
		}
	}
}

void css_selector_print(css_selector_t *selector)
{
	char path[CSS_SELECTOR_MAX_LEN];
	css_selector_node_t **sn;

	path[0] = 0;
	for (sn = selector->nodes; *sn; ++sn) {
		strcat(path, (*sn)->fullname);
		strcat(path, " ");
	}
	logger_debug("path: %s (rank = %d, batch_num = %d)\n", path,
		     selector->rank, selector->batch_num);
}

static void css_style_link_print(css_style_link_t *link, const char *selector)
{
	dict_entry_t *entry;
	dict_iterator_t *iter;
	list_node_t *node;
	char fullname[CSS_SELECTOR_MAX_LEN];

	if (selector) {
		sprintf(fullname, "%s %s", link->group->name, selector);
	} else {
		strcpy(fullname, link->group->name);
	}
	for (list_each(node, &link->styles)) {
		css_style_rule_t *snode = node->data;
		printf("\n[%s]", snode->space ? snode->space : "<none>");
		printf("[rank: %d]\n%s {\n", snode->rank, fullname);
		css_style_properties_print(snode->list);
		printf("}\n");
	}
	iter = dict_get_iterator(link->parents);
	while ((entry = dict_next(iter))) {
		css_style_link_t *parent = dict_get_val(entry);
		css_style_link_print(parent, fullname);
	}
	dict_destroy_iterator(iter);
}

void css_library_print_all(void)
{
	dict_t *group;
	css_style_link_t *link;
	css_style_link_group_t *slg;
	dict_iterator_t *iter;
	dict_entry_t *entry;

	link = NULL;
	printf("style library begin\n");
	group = list_get(&css_library.groups, 0);
	iter = dict_get_iterator(group);
	while ((entry = dict_next(iter))) {
		dict_entry_t *entry_slg;
		dict_iterator_t *iter_slg;

		slg = dict_get_val(entry);
		iter_slg = dict_get_iterator(slg->links);
		while ((entry_slg = dict_next(iter_slg))) {
			link = dict_get_val(entry_slg);
			css_style_link_print(link, NULL);
		}
		dict_destroy_iterator(iter_slg);
	}
	dict_destroy_iterator(iter);
	printf("style library end\n");
}

const css_style_decl_t *css_get_computed_style_with_cache(css_selector_t *s)
{
	list_t list;
	list_node_t *node;
	css_style_decl_t *ss;

	list_create(&list);
	ss = dict_fetch_value(css_library.cache, &s->hash);
	if (ss) {
		return ss;
	}
	ss = css_style_declaration_create();
	css_query_selector(s, &list);
	for (list_each(node, &list)) {
		css_style_rule_t *sn = node->data;
		css_style_declaration_merge_properties(ss, sn->list);
	}
	list_destroy(&list, NULL);
	dict_add(css_library.cache, &s->hash, ss);
	return ss;
}

void css_get_computed_style(css_selector_t *s, css_style_decl_t *out_ss)
{
	const css_style_decl_t *ss;

	ss = css_get_computed_style_with_cache(s);
	css_style_declaration_clear(out_ss);
	css_style_declaration_replace(out_ss, ss);
}

void css_print_style_rules_by_selector(css_selector_t *s)
{
	list_t list;
	list_node_t *node;
	css_style_decl_t *ss;
	list_create(&list);
	ss = css_style_declaration_create();
	css_query_selector(s, &list);
	printf("selector(%u) stylesheets begin\n", s->hash);
	for (list_each(node, &list)) {
		css_style_rule_t *sn = node->data;
		printf("\n[%s]", sn->space ? sn->space : "<none>");
		printf("[rank: %d]\n%s {\n", sn->rank, sn->selector);
		css_style_properties_print(sn->list);
		printf("}\n");
		css_style_declaration_merge_properties(ss, sn->list);
	}
	list_destroy(&list, NULL);
	printf("[selector(%u) final stylesheet] {\n", s->hash);
	css_style_declaration_print(ss);
	printf("}\n");
	css_style_declaration_destroy(ss);
	printf("selector(%u) stylesheets end\n", s->hash);
}

static void *names_dict_value_dup(void *privdata, const void *val)
{
	return strdup2(val);
}

static void names_dict_value_destructor(void *privdata, void *val)
{
	free(val);
}

void css_init_library(void)
{
	static dict_type_t dt = { 0 };

	dt.val_dup = NULL;
	dt.key_dup = ikey_dict_key_dup;
	dt.key_compare = ikey_dict_key_compare;
	dt.hash_function = ikey_dict_hash;
	dt.key_destructor = ikey_dict_key_destructor;
	dt.val_destructor = css_style_cache_destructor;
	dt.key_destructor = ikey_dict_key_destructor;
	css_library.cache = dict_create(&dt, NULL);
	css_library.strpool = strpool_create();
	list_create(&css_library.groups);
}

void css_destroy_library(void)
{
	dict_destroy(css_library.cache);
	strpool_destroy(css_library.strpool);
	list_destroy(&css_library.groups, (list_item_destructor_t)dict_destroy);
	css_library.strpool = NULL;
	css_library.cache = NULL;
}
