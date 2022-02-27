#include <assert.h>
#include "../include/css/style_value.h"

// https://developer.mozilla.org/en-US/docs/Web/API/CSSStyleValue/parse

css_style_value_t *css_style_value_parse(const char *property,
					 const char *css_text)
{
	// TODO
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

	for (len = 0; val->array_value[len].type == CSS_NO_VALUE; ++len)
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
	for ( i = 0; val->array_value[i].type != CSS_NO_VALUE; ++i) {
		css_style_value_destroy(&val->array_value[i]);
	}
	free(val->array_value);
}

void css_style_value_destroy(css_style_value_t *val)
{
	if (val->type == CSS_ARRAY_VALUE) {
		css_array_value_destroy(val);
	}
}
