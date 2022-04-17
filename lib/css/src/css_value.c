
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
#include "../include/css/value.h"

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
		css_keyword_value_t ident;
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
	CSS_VALDEF_PARSER_TARGET_CURLY_BRACES,
	CSS_VALDEF_PARSER_TARGET_BRACKETS,
	CSS_VALDEF_PARSER_TARGET_SIGN
} css_valdef_parser_target_t;

typedef struct css_valdef_parser_t {
	const char *cur;
	char *buffer;
	char terminator;
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
	css_style_value_t *current_value;
	size_t value_len;
	size_t index;
} css_value_matcher_t;

static struct css_value_module_t {
	/** dict_t<string, css_valdef_t> */
	dict_t *alias;

	/** dict_t<string, css_value_type_record_t> */
	dict_t *types;
} css_value;

static LCUI_BOOL css_valdef_has_children(css_valdef_t *valdef)
{
	switch (valdef->sign) {
	case CSS_VALDEF_SIGN_JUXTAPOSITION:
	case CSS_VALDEF_SIGN_DOUBLE_AMPERSAND:
	case CSS_VALDEF_SIGN_DOUBLE_BAR:
	case CSS_VALDEF_SIGN_SINGLE_BAR:
	case CSS_VALDEF_SIGN_BRACKETS:
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

static void css_valdef_destroy(css_valdef_t *valdef)
{
	if (css_valdef_has_children(valdef)) {
		list_destroy(&valdef->children,
			     (list_item_destructor_t)(css_valdef_destroy));
	}
	free(valdef);
}

static void css_valdef_shallow_destroy(css_valdef_t *valdef)
{
	if (css_valdef_has_children(valdef)) {
		list_destroy(&valdef->children, NULL);
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

#define CHECK_MAX_LEN()       \
	if (len >= max_len) { \
		break;        \
	}

size_t css_valdef_to_string(const css_valdef_t *valdef, char *str,
			    size_t max_len)
{
	size_t len = 0, i = 0;
	list_node_t *node;
	const char *name;
	char *p = str;

	switch (valdef->sign) {
	case CSS_VALDEF_SIGN_NONE:
		name = css_get_keyword_name(valdef->ident);
		if (!name) {
			name = "unknown";
		}
		len = strlen(name);
		strncpy(str, name, max_len);
		str[max_len - 1] = 0;
		len = y_min(len, max_len);
		break;
	case CSS_VALDEF_SIGN_ANGLE_BRACKET:
		CHECK_MAX_LEN();
		strncpy(p++, "<", max_len);
		name = valdef->type ? valdef->type->name : "unknown-type";
		len = strlen(name);
		len = y_min(max_len - 1, len);
		strncpy(p, name, max_len - 1);
		len++;
		CHECK_MAX_LEN();
		p = str + len;
		strncpy(p, ">", max_len - len);
		len++;
		break;
	case CSS_VALDEF_SIGN_JUXTAPOSITION:
		for (list_each(node, &valdef->children)) {
			CHECK_MAX_LEN();
			p = str + len;
			len +=
			    css_valdef_to_string(node->data, p, max_len - len);
			if (i + 1 < valdef->children.length) {
				CHECK_MAX_LEN();
				p = str + len;
				strncpy(p, " ", max_len - len);
				len++;
			}
			i++;
		}
		break;
	case CSS_VALDEF_SIGN_SINGLE_BAR:
		for (list_each(node, &valdef->children)) {
			CHECK_MAX_LEN();
			p = str + len;
			len +=
			    css_valdef_to_string(node->data, p, max_len - len);
			if (i + 1 < valdef->children.length) {
				CHECK_MAX_LEN();
				p = str + len;
				strncpy(p, " | ", max_len - len);
				len += 3;
			}
			i++;
		}
		break;
	case CSS_VALDEF_SIGN_DOUBLE_BAR:
		for (list_each(node, &valdef->children)) {
			CHECK_MAX_LEN();
			p = str + len;
			len +=
			    css_valdef_to_string(node->data, p, max_len - len);
			if (i + 1 < valdef->children.length) {
				CHECK_MAX_LEN();
				p = str + len;
				strncpy(p, " || ", max_len - len);
				len += 4;
			}
			i++;
		}
		break;
	case CSS_VALDEF_SIGN_DOUBLE_AMPERSAND:
		for (list_each(node, &valdef->children)) {
			CHECK_MAX_LEN();
			p = str + len;
			len +=
			    css_valdef_to_string(node->data, p, max_len - len);
			p = str + len;
			if (i + 1 < valdef->children.length) {
				CHECK_MAX_LEN();
				p = str + len;
				strncpy(p, " && ", max_len - len);
				len += 4;
			}
			i++;
		}
		break;
	case CSS_VALDEF_SIGN_BRACKETS:
		CHECK_MAX_LEN();
		strncpy(p, "[", max_len);
		len++;
		for (list_each(node, &valdef->children)) {
			CHECK_MAX_LEN();
			p = str + len;
			len +=
			    css_valdef_to_string(node->data, p, max_len - len);
			if (i + 1 < valdef->children.length) {
				CHECK_MAX_LEN();
				p = str + len;
				strncpy(p, " ", max_len - len);
				len++;
			}
			i++;
		}
		CHECK_MAX_LEN();
		p = str + len;
		strncpy(p, "]", max_len - len);
		len++;
		break;
	default:
		name = "unknown syntax";
		strncpy(str, name, max_len);
		len = strlen(name);
		len = y_min(len, max_len);
		break;
	}
	return len;
}

#undef CHECK_MAX_LEN

static css_valdef_t *css_valdef_shallow_copy(const css_valdef_t *valdef)
{
	list_node_t *node;
	css_valdef_t *copy;

	copy = css_valdef_create(valdef->sign);
	if (!copy) {
		return NULL;
	}
	*copy = *valdef;
	if (valdef->sign != CSS_VALDEF_SIGN_NONE &&
	    valdef->sign != CSS_VALDEF_SIGN_ANGLE_BRACKET) {
		list_create(&copy->children);
		for (list_each(node, &valdef->children)) {
			list_append(&copy->children, node->data);
		}
	}
	return copy;
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

css_valdef_parser_t *css_valdef_parser_create(size_t buffer_size,
					      char terminator)
{
	css_valdef_parser_t *parser;

	parser = calloc(sizeof(css_valdef_parser_t), 1);
	parser->buffer = calloc(sizeof(char), buffer_size);
	parser->buffer_size = buffer_size;
	parser->target = CSS_VALDEF_PARSER_TARGET_NONE;
	parser->terminator = terminator;
	list_create(&parser->valdef_parents);
	list_append(&parser->valdef_parents,
		    css_valdef_create(CSS_VALDEF_SIGN_JUXTAPOSITION));
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
	assert(parser->valdef_parents.tail.prev);
	return parser->valdef_parents.tail.prev->data;
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
	if (parent_valdef->sign == sign) {
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

static css_valdef_t *css_valdef_parser_parse(css_valdef_parser_t *parser,
					     const char *definition_str);

static int css_valdef_parser_parse_brackets(css_valdef_parser_t *parser)
{
	css_valdef_t *brackets_inner;
	css_valdef_parser_t *subparser;

	subparser = css_valdef_parser_create(parser->buffer_size, ']');
	brackets_inner = css_valdef_parser_parse(subparser, parser->cur);
	if (!brackets_inner) {
		css_valdef_parser_error(parser, subparser->error);
		css_valdef_parser_destroy(subparser);
		return -1;
	}
	parser->cur = subparser->cur;
	parser->target = CSS_VALDEF_PARSER_TARGET_NONE;
	parser->valdef = css_valdef_create(CSS_VALDEF_SIGN_BRACKETS);
	css_valdef_append(parser->valdef, brackets_inner);
	css_valdef_parser_destroy(subparser);
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
	case '{':
		break;
	case '>':
		return css_valdef_parser_error(parser, "syntax error");
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
	if (parser->valdef->source) {
		return 0;
	}
	return css_valdef_parser_error(parser, "unknown data type: `%s`\n",
				       parser->buffer);
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
	} else if (parser->pos == 2 && parser->buffer[0] == '|') {
		css_valdef_parser_commit(parser, CSS_VALDEF_SIGN_DOUBLE_BAR);
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
	case '{':
	case '}':
		return css_valdef_parser_error(parser, "syntax error");
	default:
		parser->target = CSS_VALDEF_PARSER_TARGET_KEYWORD;
		break;
	}
	return css_valdef_parser_parse_sign_end(parser);
}

static int css_valdef_parser_parse_curly_braces(css_valdef_parser_t *parser)
{
	switch (*parser->cur) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case ',':
	CASE_WHITE_SPACE:
		css_valdef_parser_get_char(parser);
		return 0;
	case '{':
		return 0;
	case '}':
		break;
	default:
		return -1;
	}
	if (sscanf(parser->buffer, "%u,%u", &parser->valdef->min_count,
		   &parser->valdef->max_count) == 2) {
		parser->target = CSS_VALDEF_PARSER_TARGET_NONE;
		return 0;
	}
	if (sscanf(parser->buffer, "%u", &parser->valdef->min_count) == 1) {
		parser->valdef->max_count = parser->valdef->min_count;
		parser->target = CSS_VALDEF_PARSER_TARGET_NONE;
		return 0;
	}
	return css_valdef_parser_error(parser, "syntax error: %s\n",
				       parser->buffer);
}

static int css_valdef_parser_resolve_target(css_valdef_parser_t *parser)
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
		parser->target = CSS_VALDEF_PARSER_TARGET_BRACKETS;
		break;
	case '{':
		css_valdef_parser_reset_target(parser);
		parser->target = CSS_VALDEF_PARSER_TARGET_CURLY_BRACES;
		break;
	case '}':
	case '>':
		return css_valdef_parser_error(parser, "syntax error");
	default:
		css_valdef_parser_reset_target(parser);
		parser->target = CSS_VALDEF_PARSER_TARGET_KEYWORD;
		break;
	}
	return 0;
}

static size_t css_valdef_parser_parse_next(css_valdef_parser_t *parser)
{
	const char *start = parser->cur;

	// printf("parse: %s\n", parser->cur);
	while (*parser->cur && *parser->cur != parser->terminator) {
		if (parser->cur >= start + parser->buffer_size) {
			++parser->cur;
			break;
		}
		switch (parser->target) {
		case CSS_VALDEF_PARSER_TARGET_NONE:
			css_valdef_parser_resolve_target(parser);
			break;
		case CSS_VALDEF_PARSER_TARGET_SIGN:
			css_valdef_parser_parse_sign(parser);
			break;
		case CSS_VALDEF_PARSER_TARGET_BRACKETS:
			css_valdef_parser_parse_brackets(parser);
			break;
		case CSS_VALDEF_PARSER_TARGET_DATA_TYPE:
			css_valdef_parser_parse_data_type(parser);
			break;
		case CSS_VALDEF_PARSER_TARGET_KEYWORD:
			css_valdef_parser_parse_keyword(parser);
			break;
		case CSS_VALDEF_PARSER_TARGET_CURLY_BRACES:
			css_valdef_parser_parse_curly_braces(parser);
			break;
		case CSS_VALDEF_PARSER_TARGET_ERROR:
			return 0;
		default:
			break;
		}
		++parser->cur;
	}
	return parser->cur - start;
}

static int css_valdef_parser_finish(css_valdef_parser_t *parser)
{
	int ret;
	css_valdef_t *parent_valdef;

	switch (parser->target) {
	case CSS_VALDEF_PARSER_TARGET_KEYWORD:
		ret = css_valdef_parser_parse_keyword_end(parser);
		break;
	case CSS_VALDEF_PARSER_TARGET_DATA_TYPE:
		ret = css_valdef_parser_parse_data_type_end(parser);
		break;
	case CSS_VALDEF_PARSER_TARGET_SIGN:
		ret = css_valdef_parser_parse_sign_end(parser);
		break;
	case CSS_VALDEF_PARSER_TARGET_ERROR:
		return -1;
	default:
		break;
	}
	parent_valdef = css_valdef_parser_get_parent_valdef(parser);
	return css_valdef_parser_commit(parser, parent_valdef->sign);
}

static css_valdef_t *css_valdef_parser_get_result(css_valdef_parser_t *parser)
{
	css_valdef_t *valdef = NULL;

	assert(parser->valdef_parents.length > 0);
	valdef = list_get_first_node(&parser->valdef_parents)->data;
	list_destroy(&parser->valdef_parents, NULL);
	return valdef;
}

static css_valdef_t *css_valdef_parser_parse(css_valdef_parser_t *parser,
					     const char *definition_str)
{
	parser->cur = definition_str;
	while (css_valdef_parser_parse_next(parser) > 0) {
		if (parser->target == CSS_VALDEF_PARSER_TARGET_ERROR) {
			return NULL;
		}
	}
	css_valdef_parser_finish(parser);
	return css_valdef_parser_get_result(parser);
}

css_valdef_t *css_compile_valdef(const char *definition_str)
{
	css_valdef_t *valdef;
	css_valdef_parser_t *parser;

	parser = css_valdef_parser_create(512, 0);
	valdef = css_valdef_parser_parse(parser, definition_str);
	css_valdef_parser_destroy(parser);
	return valdef;
}

static int css_value_matcher_resolve_next_value(css_value_matcher_t *matcher)
{
	css_style_value_t *list;
	LCUI_BOOL has_quote = FALSE;
	const char *p = matcher->cur + matcher->value_str_len;

	logger_debug(
	    "css_value_matcher_resolve_next_value(matcher<0x%p>): %s\n",
	    matcher, matcher->cur);
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

	if (matcher->value_str) {
		free(matcher->value_str);
	}
	matcher->value_str_len = p - matcher->cur;
	matcher->value_str =
	    malloc(sizeof(char) * (matcher->value_str_len + 1));
	strncpy(matcher->value_str, matcher->cur, matcher->value_str_len);
	matcher->value_str[matcher->value_str_len] = 0;

	if (matcher->value.array_value) {
		matcher->index++;
	}
	matcher->value_len++;
	list = realloc(matcher->value.array_value,
		       sizeof(css_style_value_t) * (matcher->value_len + 1));
	if (!list) {
		matcher->value_len--;
		return -1;
	}
	list[matcher->index].type = CSS_NO_VALUE;
	list[matcher->index].integer_value = 0;
	list[matcher->value_len].type = CSS_NO_VALUE;
	list[matcher->value_len].integer_value = 0;
	matcher->value.array_value = list;
	matcher->current_value = list + matcher->index;
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
	free(matcher);
}

static int css_value_matcher_match(css_value_matcher_t *matcher,
				   const css_valdef_t *valdef);

static int css_value_matcher_submatch(css_value_matcher_t *matcher,
				      const css_valdef_t *valdef);

static int css_value_matcher_match_data_type(css_value_matcher_t *matcher,
					     const css_valdef_t *valdef)
{
	if (valdef->source) {
		return css_value_matcher_submatch(matcher, valdef->source) == 0
			   ? 0
			   : -1;
	}
	if (valdef->type && valdef->type->parse_value(matcher->current_value,
						      matcher->value_str)) {
		return 0;
	}
	return -1;
}

/**
 * @see
 * https://developer.mozilla.org/en-US/docs/Web/CSS/Value_definition_syntax#double_bar
 */
static int css_value_matcher_match_double_bar(css_value_matcher_t *matcher,
					      const css_valdef_t *valdef)
{
	int ret;
	unsigned i = 0;
	list_node_t *node;
	css_valdef_t *rest_valdef;
	char str[256];

	for (list_each(node, &valdef->children)) {
		css_valdef_to_string(node->data, str, 255);
		logger_debug("[%u/%zu] matcher->value_str: %s\n", i,
			     valdef->children.length, matcher->value_str);
		if (css_value_matcher_match(matcher, node->data) != 0) {
			i++;
			logger_debug("[%u/%zu] not matched valdef: %s\n", i,
				     valdef->children.length, str);
			continue;
		}
		if (valdef->children.length < 2) {
			break;
		}
		logger_debug("[%u/%zu] matched valdef: %s\n", i,
			     valdef->children.length, str);
		rest_valdef = css_valdef_shallow_copy(valdef);
		// <border-width> || <border-style> || <border-color>
		//                          ^
		//                   matched valdef
		//
		// The sub matcher will use the remaining value definitions:
		// <border-width> || <border-color>
		list_delete(&rest_valdef->children, i);
		css_value_matcher_resolve_next_value(matcher);
		ret = css_value_matcher_submatch(matcher, rest_valdef);
		css_valdef_shallow_destroy(rest_valdef);
		if (ret == -1) {
			return -1;
		}
		break;
	}
	return 0;
}

/**
 * @see
 * https://developer.mozilla.org/en-US/docs/Web/CSS/Value_definition_syntax#double_ampersand
 */
static int css_value_matcher_match_double_ampersand(
    css_value_matcher_t *matcher, const css_valdef_t *valdef)
{
	int ret;
	unsigned i = 0;
	list_node_t *node;
	css_valdef_t *rest_valdef;
	char str[256];

	for (list_each(node, &valdef->children)) {
		css_valdef_to_string(node->data, str, 255);
		logger_debug("[%u/%zu] matcher->value_str: %s\n", i,
			     valdef->children.length, matcher->value_str);
		if (css_value_matcher_match(matcher, node->data) != 0) {
			i++;
			logger_debug("[%u/%zu] not matched valdef: %s\n", i,
				     valdef->children.length, str);
			continue;
		}
		logger_debug("[%u/%zu] matched valdef: %s\n", i,
			     valdef->children.length, str);
		if (valdef->children.length < 2) {
			return 0;
		}
		rest_valdef = css_valdef_shallow_copy(valdef);
		list_delete(&rest_valdef->children, i);
		css_value_matcher_resolve_next_value(matcher);
		ret = css_value_matcher_submatch(matcher, rest_valdef);
		css_valdef_shallow_destroy(rest_valdef);
		if (ret == 0) {
			return 0;
		}
		break;
	}
	return -1;
}

static int css_value_matcher_submatch(css_value_matcher_t *matcher,
				      const css_valdef_t *valdef)
{
	css_value_matcher_t *submatcher;

	submatcher = css_value_matcher_create(matcher->cur);
	logger_debug(
	    "css_value_matcher_submatch(matcher<0x%p>): 0x%p, cur: %s\n",
	    matcher, submatcher, submatcher->cur);
	if (submatcher->value_str_len < 1) {
		css_value_matcher_destroy(submatcher);
		return 1;
	}
	if (css_value_matcher_match(submatcher, valdef) == 0) {
		css_array_value_concat(&matcher->value, &submatcher->value);
		matcher->value_len =
		    css_array_value_get_length(&matcher->value);
		matcher->index = matcher->value_len - 1;
		matcher->value_str_len =
		    submatcher->cur + submatcher->value_str_len - matcher->cur;
		if (matcher->value_str) {
			free(matcher->value_str);
			matcher->value_str = NULL;
		}
		css_value_matcher_destroy(submatcher);
		return 0;
	}
	css_value_matcher_destroy(submatcher);
	return -1;
}

static int css_value_matcher_match_once(css_value_matcher_t *matcher,
					const css_valdef_t *valdef)
{
	size_t i = 0;
	list_node_t *node;
	char str[256];

	css_valdef_to_string(valdef, str, 256);
	str[255] = 0;
	logger_debug("css_value_matcher_match(matcher<0x%p>, \"%s\")\n",
		     matcher, str);
	switch (valdef->sign) {
	case CSS_VALDEF_SIGN_NONE:
		matcher->current_value->keyword_value =
		    css_get_keyword_key(matcher->value_str);
		if (valdef->ident != matcher->current_value->keyword_value) {
			return -1;
		}
		matcher->current_value->type = CSS_KEYWORD_VALUE;
		break;
	case CSS_VALDEF_SIGN_ANGLE_BRACKET:
		return css_value_matcher_match_data_type(matcher, valdef);
	case CSS_VALDEF_SIGN_JUXTAPOSITION:
	case CSS_VALDEF_SIGN_BRACKETS:
		for (list_each(node, &valdef->children)) {
			if (i > 0) {
				css_value_matcher_resolve_next_value(matcher);
			}
			if (css_value_matcher_match(matcher, node->data) != 0) {
				return -1;
			}
			i++;
		}
		break;
	case CSS_VALDEF_SIGN_SINGLE_BAR:
		for (list_each(node, &valdef->children)) {
			css_valdef_to_string(node->data, str, 256);
			if (css_value_matcher_match(matcher, node->data) == 0) {
				logger_debug("[%u/%zu] matched valdef: %s\n", i,
					     valdef->children.length, str);
				break;
			}
			logger_debug("[%u/%zu] not matched valdef: %s\n", i,
				     valdef->children.length, str);
			++i;
		}
		return -1;
	case CSS_VALDEF_SIGN_DOUBLE_BAR:
		if (css_value_matcher_match_double_bar(matcher, valdef) != 0) {
			return -1;
		}
		break;
	case CSS_VALDEF_SIGN_DOUBLE_AMPERSAND:
		if (css_value_matcher_match_double_ampersand(matcher, valdef) !=
		    0) {
			return -1;
		}
		break;
	default:
		return -1;
	}
	return 0;
}

static int css_value_matcher_match(css_value_matcher_t *matcher,
				   const css_valdef_t *valdef)
{
	unsigned i;

	for (i = 0; i < valdef->max_count; ++i) {
		if (i > 0) {
			css_value_matcher_resolve_next_value(matcher);
		}
		if (css_value_matcher_match_once(matcher, valdef) != 0) {
			break;
		}
	}
	return i >= valdef->min_count ? 0 : -1;
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
		char str[1024] = { 0 };
		css_valdef_to_string(valdef, str, 1023);
		printf("%s\n", str);
		return dict_add(css_value.alias, (void *)alias, valdef);
	}
	return -3;
}
