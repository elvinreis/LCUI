#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../include/css/keywords.h"
#include "../include/css/style_value.h"
#include "../include/css/library.h"

// https://developer.mozilla.org/en-US/docs/Web/API/CSSStyleValue/parse

css_style_value_t *css_style_value_parse(const char *property,
					 const char *css_text)
{
	// TODO
	return NULL;
}

css_style_value_t *css_array_value_create(size_t len)
{
	size_t i;
	css_style_value_t *val;

	val = calloc(len + 1, sizeof(css_style_value_t));
	for (i = 0; i < len; ++i) {
		val[i].type = CSS_INVALID_VALUE;
	}
	val[len].type = CSS_NO_VALUE;
	return val;
}

size_t css_array_value_get_length(css_style_value_t *val)
{
	size_t len;

	for (len = 0; val->array_value[len].type != CSS_NO_VALUE; ++len)
		;
	return len;
}

int css_array_value_set_length(css_style_value_t *val, size_t new_len)
{
	size_t len;
	css_style_value_t *arr;

	arr = realloc(val->array_value,
		      (new_len + 1) * sizeof(css_style_value_t));
	if (!arr) {
		return -1;
	}
	val->array_value = arr;
	for (len = css_array_value_get_length(val); len < new_len; ++len) {
		val->array_value[len].type = CSS_INVALID_VALUE;
	}
	val->array_value[new_len].type = CSS_NO_VALUE;
	return 0;
}

int css_array_value_concat(css_style_value_t *val1, css_style_value_t *val2)
{
	size_t len1, len2, new_len;

	assert(val1->type == CSS_ARRAY_VALUE && val2->type == CSS_ARRAY_VALUE);
	len1 = css_array_value_get_length(val1);
	len2 = css_array_value_get_length(val2);
	new_len = len1 + len2;
	if (css_array_value_set_length(val1, new_len) != 0) {
		return -1;
	}
	for (len2 = 0; len1 < new_len; ++len1, ++len2) {
		val1->array_value[len1] = val2->array_value[len2];
	}
	free(val2->array_value);
	val2->array_value = NULL;
	return 0;
}

void css_array_value_destroy(css_style_value_t *val)
{
	size_t i;

	if (!val->array_value) {
		return;
	}
	for (i = 0; val->array_value[i].type != CSS_NO_VALUE; ++i) {
		css_style_value_destroy(&val->array_value[i]);
	}
	free(val->array_value);
}

void css_style_value_destroy(css_style_value_t *val)
{
	switch (val->type) {
	case CSS_ARRAY_VALUE:
		css_array_value_destroy(val);
		break;
	case CSS_UNPARSED_VALUE:
		free(val->unparsed_value);
		break;
	case CSS_STRING_VALUE:
		free(val->string_value);
		val->string_value = NULL;
		break;
	default:
		break;
	}
	val->type = CSS_NO_VALUE;
}

void css_style_value_merge(css_style_value_t *dst, css_style_value_t *src)
{
	switch (src->type) {
	case CSS_IMAGE_VALUE:
	case CSS_STRING_VALUE:
	case CSS_UNPARSED_VALUE:
		dst->string_value = strdup2(src->string_value);
		break;
	default:
		*dst = *src;
		break;
	}
	dst->type = src->type;
}

void css_style_value_to_string(css_style_value_t *s, char *outstr,
			       size_t max_len)
{
	switch (s->type) {
	case CSS_INVALID_VALUE:
		strncpy(outstr, "<invalid value>", max_len);
		break;
	case CSS_COLOR_VALUE:
		if (s->color_value.a < 255) {
			snprintf(outstr, max_len, "rgba(%d,%d,%d,%g)",
				 s->color_value.r, s->color_value.g,
				 s->color_value.b, s->color_value.a / 255.0);
		} else {
			snprintf(outstr, max_len, "#%02x%02x%02x",
				 s->color_value.r, s->color_value.g,
				 s->color_value.b);
		}
		break;
	case CSS_IMAGE_VALUE:
		snprintf(outstr, max_len, "image(\"%s\")", s->image_value);
		break;
	case CSS_STRING_VALUE:
		strncpy(outstr, s->string_value, max_len);
		break;
	case CSS_KEYWORD_VALUE:
		strncpy(outstr, css_get_keyword_name(s->keyword_value),
			max_len);
		break;
	case CSS_UNIT_VALUE:
		snprintf(outstr, max_len, "%g%s", s->unit_value.value,
			 s->unit_value.unit);
		break;
	case CSS_UNPARSED_VALUE:
		strncpy(outstr, s->unparsed_value, max_len);
		break;
	default:
		break;
	}
}
