#include "../include/css/keywords.h"

/** 样式字符串值与标识码 */
typedef struct css_keyword_t {
	int key;
	char *name;
} css_keyword_t;

static struct css_keywords_module_t {
	/**
	 * 索引表，以值索引
	 * dict_t<int, css_keyword_t*>
	 */
	dict_t *key_map;

	/**
	 * 名称索引表，以名称索引
	 * dict_t<string, css_keyword_t*>
	 */
	dict_t *name_map;

} css_keywords;

static uint64_t ikey_dict_hash(const void *key)
{
	return (*(unsigned int *)key);
}

static void keyword_destroy(void *data)
{
	css_keyword_t *kw = data;
	free(kw->name);
	free(kw);
}

static int ikey_dict_key_compare(void *privdata, const void *key1,
				 const void *key2)
{
	return *(unsigned int *)key1 == *(unsigned int *)key2;
}

static void keyword_destructor(void *privdata, void *data)
{
	keyword_destroy(data);
}

static css_keyword_t *keyword_create(int key, const char *name)
{
	css_keyword_t *kw;
	kw = malloc(sizeof(css_keyword_t));
	kw->name = strdup2(name);
	kw->key = key;
	return kw;
}

int css_register_keyword(int key, const char *name)
{
	css_keyword_t *kw = keyword_create(key, name);
	if (dict_add(css_keywords.key_map, kw->name, kw)) {
		keyword_destroy(kw);
		return -1;
	}
	if (dict_add(css_keywords.name_map, &kw->key, kw)) {
		keyword_destroy(kw);
		return -2;
	}
	return 0;
}

int css_get_keyword_key(const char *str)
{
	css_keyword_t *kw;
	kw = dict_fetch_value(css_keywords.key_map, str);
	if (kw) {
		return kw->key;
	}
	return -1;
}

const char *css_get_keyword_name(int val)
{
	css_keyword_t *kw;
	kw = dict_fetch_value(css_keywords.name_map, &val);
	if (kw) {
		return kw->name;
	}
	return NULL;
}

void css_init_keywords(void)
{
	static dict_type_t keys_dt = { 0 };
	static dict_type_t names_dt = { 0 };

	names_dt.key_compare = ikey_dict_key_compare;
	names_dt.hash_function = ikey_dict_hash;
	dict_init_string_key_type(&keys_dt);
	keys_dt.val_destructor = keyword_destructor;
	css_keywords.key_map = dict_create(&keys_dt, NULL);
	css_keywords.name_map = dict_create(&names_dt, NULL);

	css_register_keyword(CSS_KEYWORD_NONE, "none");
	css_register_keyword(CSS_KEYWORD_AUTO, "auto");
	css_register_keyword(CSS_KEYWORD_INHERIT, "inherit");
	css_register_keyword(CSS_KEYWORD_INITIAL, "initial");
	css_register_keyword(CSS_KEYWORD_CONTAIN, "contain");
	css_register_keyword(CSS_KEYWORD_COVER, "cover");
	css_register_keyword(CSS_KEYWORD_LEFT, "left");
	css_register_keyword(CSS_KEYWORD_CENTER, "center");
	css_register_keyword(CSS_KEYWORD_RIGHT, "right");
	css_register_keyword(CSS_KEYWORD_TOP, "top");
	css_register_keyword(CSS_KEYWORD_TOP_LEFT, "top left");
	css_register_keyword(CSS_KEYWORD_TOP_CENTER, "top center");
	css_register_keyword(CSS_KEYWORD_TOP_RIGHT, "top right");
	css_register_keyword(CSS_KEYWORD_MIDDLE, "middle");
	css_register_keyword(CSS_KEYWORD_CENTER_LEFT, "center left");
	css_register_keyword(CSS_KEYWORD_CENTER_CENTER, "center center");
	css_register_keyword(CSS_KEYWORD_CENTER_RIGHT, "center right");
	css_register_keyword(CSS_KEYWORD_BOTTOM, "bottom");
	css_register_keyword(CSS_KEYWORD_BOTTOM_LEFT, "bottom left");
	css_register_keyword(CSS_KEYWORD_BOTTOM_CENTER, "bottom center");
	css_register_keyword(CSS_KEYWORD_BOTTOM_RIGHT, "bottom right");
	css_register_keyword(CSS_KEYWORD_SOLID, "solid");
	css_register_keyword(CSS_KEYWORD_DOTTED, "dotted");
	css_register_keyword(CSS_KEYWORD_DOUBLE, "double");
	css_register_keyword(CSS_KEYWORD_DASHED, "dashed");
	css_register_keyword(CSS_KEYWORD_CONTENT_BOX, "content-box");
	css_register_keyword(CSS_KEYWORD_PADDING_BOX, "padding-box");
	css_register_keyword(CSS_KEYWORD_BORDER_BOX, "border-box");
	css_register_keyword(CSS_KEYWORD_GRAPH_BOX, "graph-box");
	css_register_keyword(CSS_KEYWORD_STATIC, "static");
	css_register_keyword(CSS_KEYWORD_RELATIVE, "relative");
	css_register_keyword(CSS_KEYWORD_ABSOLUTE, "absolute");
	css_register_keyword(CSS_KEYWORD_BLOCK, "block");
	css_register_keyword(CSS_KEYWORD_INLINE_BLOCK, "inline-block");
	css_register_keyword(CSS_KEYWORD_FLEX, "flex");
	css_register_keyword(CSS_KEYWORD_NORMAL, "normal");
	css_register_keyword(CSS_KEYWORD_FLEX_START, "flex-start");
	css_register_keyword(CSS_KEYWORD_FLEX_END, "flex-end");
	css_register_keyword(CSS_KEYWORD_STRETCH, "stretch");
	css_register_keyword(CSS_KEYWORD_SPACE_BETWEEN, "space-between");
	css_register_keyword(CSS_KEYWORD_SPACE_AROUND, "space-around");
	css_register_keyword(CSS_KEYWORD_SPACE_EVENLY, "space-evenly");
	css_register_keyword(CSS_KEYWORD_NOWRAP, "nowrap");
	css_register_keyword(CSS_KEYWORD_WRAP, "wrap");
	css_register_keyword(CSS_KEYWORD_ROW, "row");
	css_register_keyword(CSS_KEYWORD_COLUMN, "column");
}

void css_destroy_keywords(void)
{
	dict_destroy(css_keywords.name_map);
	dict_destroy(css_keywords.key_map);
	css_keywords.name_map = NULL;
	css_keywords.key_map = NULL;
}
