#ifndef LIBCSS_INCLUDE_CSS_STYLE_VALUE_H
#define LIBCSS_INCLUDE_CSS_STYLE_VALUE_H

#include <LCUI/header.h>
#include "def.h"

LCUI_BEGIN_HEADER

LCUI_API css_style_value_t *css_array_value_create(size_t len);

LCUI_API size_t css_array_value_get_length(css_style_value_t *val);

LCUI_API int css_array_value_set_length(css_style_value_t *val, size_t new_len);

LCUI_API int css_array_value_concat(css_style_value_t *val1,
				    css_style_value_t *val2);

LCUI_API void css_style_value_merge(css_style_value_t *dst,
				    css_style_value_t *src);

LCUI_API void css_style_value_to_string(css_style_value_t *s, char *outstr,
					size_t max_len);

LCUI_API void css_array_value_destroy(css_style_value_t *val);

LCUI_API void css_style_value_destroy(css_style_value_t *val);

LCUI_END_HEADER

#endif
