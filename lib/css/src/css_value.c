
/**
 * @see https://developer.mozilla.org/en-US/docs/Web/CSS/Value_definition_syntax
 * @see https://drafts.csswg.org/css-values/#value-defs
 * @see https://developer.mozilla.org/en-US/docs/Web/API/CSS/RegisterProperty
 **/

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include "../include/css/keywords.h"
#include "../include/css/library.h"
#include "../include/css/style_value.h"

#define CSS_VALDEF_PARSER_ERROR_SIZE 256

typedef enum css_valdef_sign_t {
	CSS_VALDEF_SIGN_NONE,
	CSS_VALDEF_SIGN_JUXTAPOSITION,
	CSS_VALDEF_SIGN_DOUBLE_AMPERSAND,
	CSS_VALDEF_SIGN_DOUBLE_BAR,
	CSS_VALDEF_SIGN_SINGLE_BAR,
	CSS_VALDEF_SIGN_BRACKETS,
	CSS_VALDEF_SIGN_ANGLE_BRACKET
} css_valdef_sign_t;

struct css_value_type_record_t {
	char *name;
	css_value_parse_func_t parse_value;
};

struct css_valdef_t {
	css_valdef_sign_t sign;
	unsigned min_count;
	unsigned max_count;
	const css_valdef_t *source;
	union {
		int ident;
		/** list_t<css_valdef_t> */
		list_t children;
		const css_value_type_record_t *type;
	};
};

typedef enum css_valdef_parser_target_t {
	CSS_VALDEF_PARSER_TARGET_NONE,
	CSS_VALDEF_PARSER_TARGET_ERROR,
	CSS_VALDEF_PARSER_TARGET_KEYWORD,
	CSS_VALDEF_PARSER_TARGET_DATA_TYPE,
	CSS_VALDEF_PARSER_TARGET_SIGN
} css_valdef_parser_target_t;

typedef struct css_valdef_parser_t {
	const char *cur;
	char *buffer;
	size_t pos;
	size_t buffer_size;
	css_valdef_parser_target_t target;
	char error[CSS_VALDEF_PARSER_ERROR_SIZE];

	css_valdef_t *valdef;

	/** list_t<css_valdef_parser_target_t> */
	list_t valdef_parents;
} css_valdef_parser_t;

typedef struct css_value_matcher_t {
	const char *cur;

	char *value_str;
	size_t value_str_len;

	css_style_value_t value;
	unsigned value_len;
	unsigned index;
} css_value_matcher_t;

static struct css_value_module_t {
	/** dict_t<string, css_valdef_t> */
	dict_t *alias;

	/** dict_t<string, css_value_type_record_t> */
	dict_t *types;
} css_value;

static void css_valdef_destroy(css_valdef_t *valdef)
{
	switch (valdef->sign) {
	case CSS_VALDEF_SIGN_DOUBLE_BAR:
	case CSS_VALDEF_SIGN_SINGLE_BAR:
		list_destroy(&valdef->children,
			     (list_item_destructor_t)(css_valdef_destroy));
		break;
	default:
		break;
	}
	free(valdef);
}

static void css_value_alias_destroy_value(void *priv, void *val)
{
	css_valdef_destroy(val);
}

static void css_value_types_destroy_value(void *priv, void *val)
{
	css_value_type_record_t *t = val;

	free(t->name);
	free(t);
}

void css_init_value_definitons(void)
{
	static dict_type_t alias_dt;
	static dict_type_t types_dt;

	dict_init_string_copy_key_type(&alias_dt);
	alias_dt.val_destructor = css_value_alias_destroy_value;
	css_value.alias = dict_create(&alias_dt, NULL);

	dict_init_string_key_type(&types_dt);
	types_dt.val_destructor = css_value_types_destroy_value;
	css_value.types = dict_create(&types_dt, NULL);
}

void css_destroy_value_definitons(void)
{
	dict_destroy(css_value.alias);
	dict_destroy(css_value.types);
	css_value.types = NULL;
	css_value.alias = NULL;
}

static css_valdef_t *css_valdef_create(css_valdef_sign_t sign)
{
	css_valdef_t *valdef;

	valdef = calloc(1, sizeof(css_valdef_t));
	if (!valdef) {
		return NULL;
	}
	valdef->max_count = 1;
	valdef->min_count = 1;
	valdef->sign = sign;
	return valdef;
}

static void css_valdef_append(css_valdef_t *valdef, css_valdef_t *child)
{
	assert(valdef->sign != CSS_VALDEF_SIGN_NONE &&
	       valdef->sign != CSS_VALDEF_SIGN_ANGLE_BRACKET);
	list_append(&valdef->children, child);
}

const css_value_type_record_t *css_register_value_type(
    const char *type_name, css_value_parse_func_t parse)
{
	css_value_type_record_t *t;

	t = malloc(sizeof(css_value_type_record_t));
	if (!t) {
		return NULL;
	}
	t->name = strdup2(type_name);
	t->parse_value = parse;
	if (dict_add(css_value.types, t->name, t) != 0) {
		free(t->name);
		free(t);
		return NULL;
	}
	return t;
}

const css_value_type_record_t *css_get_value_type(const char *type_name)
{
	return dict_fetch_value(css_value.types, type_name);
}

const css_valdef_t *css_resolve_valdef_alias(const char *alias)
{
	return dict_fetch_value(css_value.alias, alias);
}

css_valdef_parser_t *css_valdef_parser_create(size_t buffer_size)
{
	css_valdef_parser_t *parser;

	parser = calloc(sizeof(css_valdef_parser_t), 1);
	parser->buffer = calloc(sizeof(char), buffer_size);
	parser->buffer_size = buffer_size;
	parser->target = CSS_VALDEF_PARSER_TARGET_NONE;
	parser->valdef = NULL;
	list_create(&parser->valdef_parents);
	return parser;
}

void css_valdef_parser_destroy(css_valdef_parser_t *parser)
{
	free(parser->buffer);
	free(parser);
}

INLINE void css_valdef_parser_get_char(css_valdef_parser_t *parser)
{
	parser->buffer[parser->pos++] = *(parser->cur);
	parser->buffer[parser->pos] = 0;
}

static int css_valdef_parser_error(css_valdef_parser_t *parser, const char *fmt,
				   ...)
{
	size_t len;
	va_list args;

	va_start(args, fmt);
	len = vsnprintf(parser->error, CSS_VALDEF_PARSER_ERROR_SIZE, fmt, args);
	va_end(args);
	parser->buffer[CSS_VALDEF_PARSER_ERROR_SIZE - 1] = 0;
	parser->target = CSS_VALDEF_PARSER_TARGET_ERROR;
	return -1;
}

INLINE css_valdef_t *css_valdef_parser_get_parent_valdef(
    css_valdef_parser_t *parser)
{
	return parser->valdef_parents.tail.prev
		   ? parser->valdef_parents.tail.prev->data
		   : NULL;
}

INLINE void css_valdef_parser_reset_target(css_valdef_parser_t *parser)
{
	parser->target = CSS_VALDEF_PARSER_TARGET_NONE;
	parser->buffer[0] = 0;
	parser->pos = 0;
	parser->cur--;
}

// 在遇到符号时提交当前已解析的值定义
static int css_valdef_parser_commit(css_valdef_parser_t *parser,
				    css_valdef_sign_t sign)
{
	css_valdef_t *parent_valdef;
	css_valdef_t *child;

	// TODO: 处理 && 和 || 混写情况
	// TODO: 处理非数组值的情况

	if (!parser->valdef) {
		return 0;
	}
	parent_valdef = css_valdef_parser_get_parent_valdef(parser);
	if (parent_valdef && parent_valdef->sign == sign) {
		css_valdef_append(parent_valdef, parser->valdef);
	} else {
		// Before:
		// [left | right &&]
		//               ^
		//               |
		//         current sign
		// After:
		// [left | [right && ]]
		child = css_valdef_create(sign);
		css_valdef_append(child, parser->valdef);
		if (parent_valdef) {
			// Before:
			// left |
			//      ^
			//      |
			// current sign
			//
			// After:
			// [left | ]
			css_valdef_append(parent_valdef, child);
		}
		list_append(&parser->valdef_parents, child);
	}
	parser->valdef = NULL;
	return 0;
}

static int css_valdef_parser_open_bracket(css_valdef_parser_t *parser)
{
	css_valdef_t *parent_valdef;
	css_valdef_t *group;

	group = css_valdef_create(CSS_VALDEF_SIGN_BRACKETS);
	if (!group) {
		return -1;
	}
	if (parser->valdef) {
		// Example:
		// <length> [none | auto]
		//          ^
		//          |
		//         cur
		css_valdef_parser_commit(parser, CSS_VALDEF_SIGN_JUXTAPOSITION);
	}
	parent_valdef = css_valdef_parser_get_parent_valdef(parser);
	if (parent_valdef) {
		css_valdef_append(parent_valdef, group);
	}
	list_append(&parser->valdef_parents, group);
	return 0;
}

static int css_valdef_parser_close_bracket(css_valdef_parser_t *parser)
{
	css_valdef_t *parent_valdef;

	parent_valdef = css_valdef_parser_get_parent_valdef(parser);
	if (!parent_valdef) {
		return css_valdef_parser_error(parser, "syntax error");
	}
	// Example:
	// <length> [none]
	//               ^
	//               |
	//              cur
	css_valdef_parser_commit(parser, parent_valdef->sign);
	while (parent_valdef &&
	       parent_valdef->sign != CSS_VALDEF_SIGN_BRACKETS) {
		list_delete_last(&parser->valdef_parents);
		parent_valdef = css_valdef_parser_get_parent_valdef(parser);
	}
	if (!parent_valdef) {
		return css_valdef_parser_error(parser, "syntax error");
	}
	list_delete_last(&parser->valdef_parents);
	return 0;
}

static int css_valdef_parser_parse_keyword_end(css_valdef_parser_t *parser)
{
	// Example:
	// top left | right
	//         ^
	//         |
	//        cur
	parser->valdef = css_valdef_create(CSS_VALDEF_SIGN_NONE);
	parser->valdef->ident = css_get_keyword_key(parser->buffer);
	if (parser->valdef->ident == -1) {
		return css_valdef_parser_error(
		    parser, "unknown keyword: `%s`\n", parser->buffer);
	}
	css_valdef_parser_reset_target(parser);
	return 0;
}

static int css_valdef_parser_parse_keyword(css_valdef_parser_t *parser)
{
	switch (*parser->cur) {
	CASE_WHITE_SPACE:
	case '|':
	case '&':
	case '[':
	case ']':
	case '<':
		break;
	case '>':
		parser->target = CSS_VALDEF_PARSER_TARGET_ERROR;
		return -1;
	default:
		css_valdef_parser_get_char(parser);
		return 0;
	}
	return css_valdef_parser_parse_keyword_end(parser);
}

static int css_valdef_parser_parse_data_type_end(css_valdef_parser_t *parser)
{
	parser->target = CSS_VALDEF_PARSER_TARGET_NONE;
	parser->valdef = css_valdef_create(CSS_VALDEF_SIGN_ANGLE_BRACKET);
	parser->valdef->source = NULL;
	parser->valdef->type = css_get_value_type(parser->buffer);
	if (parser->valdef->type) {
		return 0;
	}
	parser->valdef->source = css_resolve_valdef_alias(parser->buffer);
	if (!parser->valdef->source) {
		return css_valdef_parser_error(
		    parser, "unknown data type: `%s`\n", parser->buffer);
	}
	return 0;
}

static int css_valdef_parser_parse_data_type(css_valdef_parser_t *parser)
{
	switch (*parser->cur) {
	case '<':
	case '&':
	case '?':
	case '|':
	case '{':
	case '}':
	case '[':
	case ']':
	CASE_WHITE_SPACE:
		return css_valdef_parser_error(parser, "syntax error");
	case '>':
		break;
	default:
		css_valdef_parser_get_char(parser);
		return 0;
	}
	return css_valdef_parser_parse_data_type_end(parser);
}

static int css_valdef_parser_parse_sign_end(css_valdef_parser_t *parser)
{
	if (parser->pos == 0) {
		css_valdef_parser_commit(parser, CSS_VALDEF_SIGN_JUXTAPOSITION);
	} else if (parser->pos == 1 && parser->buffer[0] == '|') {
		css_valdef_parser_commit(parser, CSS_VALDEF_SIGN_SINGLE_BAR);
	} else if (parser->pos == 2 && parser->buffer[0] == '&') {
		css_valdef_parser_commit(parser,
					 CSS_VALDEF_SIGN_DOUBLE_AMPERSAND);
	} else {
		return css_valdef_parser_error(parser, "unknown sign: `%s`\n",
					       parser->buffer);
	}
	if (parser->target == CSS_VALDEF_PARSER_TARGET_KEYWORD ||
	    parser->target == CSS_VALDEF_PARSER_TARGET_NONE) {
		css_valdef_parser_reset_target(parser);
	}
	return 0;
}

static int css_valdef_parser_parse_sign(css_valdef_parser_t *parser)
{
	switch (*parser->cur) {
	case '|':
	case '&':
		css_valdef_parser_get_char(parser);
		switch (parser->pos) {
		case 1:
			break;
		case 2:
			if (parser->buffer[parser->pos] ==
			    parser->buffer[parser->pos - 1]) {
				break;
			}
		default:
			break;
		}
		return 0;
	case '[':
	case ']':
		parser->target = CSS_VALDEF_PARSER_TARGET_NONE;
		break;
	CASE_WHITE_SPACE:
		// Example:
		// none | left
		//       ^
		//       |
		//      cur
		return 0;
	default:
		parser->target = CSS_VALDEF_PARSER_TARGET_KEYWORD;
		break;
	}
	return css_valdef_parser_parse_sign_end(parser);
}

static int css_valdef_parser_parse_target(css_valdef_parser_t *parser)
{
	switch (*parser->cur) {
	case '|':
	case '&':
	CASE_WHITE_SPACE:
		css_valdef_parser_reset_target(parser);
		parser->target = CSS_VALDEF_PARSER_TARGET_SIGN;
		break;
	case '<':
		parser->target = CSS_VALDEF_PARSER_TARGET_DATA_TYPE;
		break;
	case '[':
		// TODO:
		// none | [left | right]
		return css_valdef_parser_open_bracket(parser);
	case ']':
		return css_valdef_parser_close_bracket(parser);
	case '{':
	case '}':
	case '>':
		parser->target = CSS_VALDEF_PARSER_TARGET_ERROR;
		return -1;
	default:
		css_valdef_parser_reset_target(parser);
		parser->target = CSS_VALDEF_PARSER_TARGET_KEYWORD;
		break;
	}
	return 0;
}

static size_t css_valdef_parser_parse(css_valdef_parser_t *parser,
				      const char *str)
{
	size_t size = 0;

	parser->cur = str;
	while (*parser->cur && size < parser->buffer_size) {
		switch (parser->target) {
		case CSS_VALDEF_PARSER_TARGET_NONE:
			css_valdef_parser_parse_target(parser);
			break;
		case CSS_VALDEF_PARSER_TARGET_SIGN:
			css_valdef_parser_parse_sign(parser);
			break;
		case CSS_VALDEF_PARSER_TARGET_DATA_TYPE:
			css_valdef_parser_parse_data_type(parser);
			break;
		case CSS_VALDEF_PARSER_TARGET_KEYWORD:
			css_valdef_parser_parse_keyword(parser);
			break;
		case CSS_VALDEF_PARSER_TARGET_ERROR:
			break;
		default:
			break;
		}
		++parser->cur;
		++size;
	}
	return size;
}

static int css_valdef_parser_finish(css_valdef_parser_t *parser)
{
	switch (parser->target) {
	case CSS_VALDEF_PARSER_TARGET_KEYWORD:
		return css_valdef_parser_parse_keyword_end(parser);
	case CSS_VALDEF_PARSER_TARGET_DATA_TYPE:
		return css_valdef_parser_parse_data_type_end(parser);
	case CSS_VALDEF_PARSER_TARGET_SIGN:
		return css_valdef_parser_parse_sign_end(parser);
	case CSS_VALDEF_PARSER_TARGET_ERROR:
		return -1;
	default:
		break;
	}
	return 0;
}

css_valdef_t *css_compile_valdef(const char *definition_str)
{
	size_t len;
	const char *p;
	css_valdef_t *valdef;
	css_valdef_parser_t *parser;

	parser = css_valdef_parser_create(512);
	for (len = 1, p = definition_str; len > 0; p += len) {
		len = css_valdef_parser_parse(parser, p);
		if (parser->target == CSS_VALDEF_PARSER_TARGET_ERROR) {
			css_valdef_parser_destroy(parser);
			return NULL;
		}
	}
	css_valdef_parser_finish(parser);
	valdef = parser->valdef;
	parser->valdef = NULL;
	css_valdef_parser_destroy(parser);
	return valdef;
}

static int css_value_matcher_resolve_next_value(css_value_matcher_t *matcher)
{
	css_style_value_t *list;
	LCUI_BOOL has_quote = FALSE;
	const char *p = matcher->cur;

	while (*p) {
		switch (*p) {
		CASE_WHITE_SPACE:
			break;
		case '"':
			has_quote = !has_quote;
			break;
		default:
			if (!has_quote) {
				goto resolve_value_str_tail;
			}
			break;
		}
		p++;
	}

resolve_value_str_tail:

	matcher->cur = p;
	while (*p) {
		switch (*p) {
		CASE_WHITE_SPACE:
			if (!has_quote) {
				goto copy_value_str;
			}
			break;
		case '"':
			has_quote = !has_quote;
			break;
		default:
			break;
		}
		p++;
	}

copy_value_str:

	free(matcher->value_str);
	matcher->value_str_len = p - matcher->cur;
	matcher->value_str =
	    malloc(sizeof(char) * (matcher->value_str_len + 1));
	strncpy(matcher->value_str, matcher->cur, matcher->value_str_len);
	matcher->value_str[matcher->value_str_len] = 0;

	list = realloc(matcher->value.array_value,
		       sizeof(css_style_value_t) * (matcher->value_len + 1));
	if (!list) {
		matcher->value_len--;
		return -1;
	}
	if (matcher->value_len > 0) {
		matcher->index++;
	} else {
		list[0].type = CSS_NO_VALUE;
		list[0].integer_value = 0;
	}
	list[matcher->value_len].type = CSS_NO_VALUE;
	list[matcher->value_len].integer_value = 0;
	matcher->value.array_value = list;
	return 0;
}

css_value_matcher_t *css_value_matcher_create(const char *str)
{
	css_value_matcher_t *matcher;

	matcher = calloc(1, sizeof(css_value_matcher_t));
	if (!matcher) {
		return NULL;
	}
	matcher->cur = str;
	matcher->value.type = CSS_ARRAY_VALUE;
	css_value_matcher_resolve_next_value(matcher);
	return matcher;
}

static void css_value_matcher_destroy(css_value_matcher_t *matcher)
{
	matcher->index = 0;
	matcher->cur = NULL;
	matcher->value_len = 0;
	css_style_value_destroy(&matcher->value);
	free(matcher->value_str);
	matcher->value_str = NULL;
	matcher->value_str_len = 0;
}

static int css_value_matcher_match(css_value_matcher_t *matcher,
				   const css_valdef_t *valdef);

static int css_value_matcher_match_data_type(css_value_matcher_t *matcher,
					     const css_valdef_t *valdef)
{
	css_value_matcher_t *submatcher;

	if (valdef->source) {
		submatcher = css_value_matcher_create(matcher->cur);
		if (css_value_matcher_match(submatcher, valdef->source) != 0) {
			return -1;
		}
		matcher->index += submatcher->index;
		matcher->cur = submatcher->cur + submatcher->value_str_len;
		css_array_value_concat(&matcher->value, &submatcher->value);
		css_value_matcher_destroy(submatcher);
		return css_value_matcher_resolve_next_value(matcher);
	}
	if (!valdef->type || valdef->type->parse_value(
				 matcher->value.array_value + matcher->index,
				 matcher->value_str) != 0) {
		return -1;
	}
	return css_value_matcher_resolve_next_value(matcher);
}

static int css_value_matcher_match(css_value_matcher_t *matcher,
				   const css_valdef_t *valdef)
{
	list_node_t *node;

	switch (valdef->sign) {
	case CSS_VALDEF_SIGN_NONE:
		if (valdef->ident != css_get_keyword_key(matcher->value_str)) {
			return -1;
		}
		return 0;
	case CSS_VALDEF_SIGN_ANGLE_BRACKET:
		return css_value_matcher_match_data_type(matcher, valdef);
	case CSS_VALDEF_SIGN_JUXTAPOSITION:
		for (list_each(node, &valdef->children)) {
			if (css_value_matcher_match(matcher, node->data) != 0) {
				return -1;
			}
		}
		return 0;
	case CSS_VALDEF_SIGN_SINGLE_BAR:
		for (list_each(node, &valdef->children)) {
			if (css_value_matcher_match(matcher, node->data) == 0) {
				return 0;
			}
		}
		return -1;
	case CSS_VALDEF_SIGN_DOUBLE_BAR:
	case CSS_VALDEF_SIGN_DOUBLE_AMPERSAND:
	case CSS_VALDEF_SIGN_BRACKETS:
		// TODO
	default:
		return -1;
	}
}

int css_parse_value(const css_valdef_t *valdef, const char *str,
		    css_style_value_t *val)
{
	int ret;
	css_value_matcher_t *matcher;

	matcher = css_value_matcher_create(str);
	if (!matcher) {
		return -1;
	}
	ret = css_value_matcher_match(matcher, valdef);
	if (ret == 0) {
		*val = matcher->value;
		matcher->value.type = CSS_NO_VALUE;
	}
	css_value_matcher_destroy(matcher);
	return ret;
}

int css_register_valdef_alias(const char *alias, const char *definitons)
{
	css_valdef_t *valdef;

	if (css_get_keyword_key(alias) >= 0) {
		return -1;
	}
	valdef = css_compile_valdef(definitons);
	if (valdef) {
		return dict_add(css_value.alias, (void *)alias, valdef);
	}
	return -3;
}
