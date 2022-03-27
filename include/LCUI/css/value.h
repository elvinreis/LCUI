#ifndef LIBCSS_INCLUDE_CSS_VALUE_H
#define LIBCSS_INCLUDE_CSS_VALUE_H

#include <LCUI/header.h>
#include "def.h"

LCUI_BEGIN_HEADER

LCUI_API void css_init_value_definitons(void);

LCUI_API void css_destroy_value_definitons(void);

LCUI_API const css_value_type_record_t *css_register_value_type(
    const char *type_name, css_value_parse_func_t parse);

LCUI_API const css_value_type_record_t *css_get_value_type(
    const char *type_name);

LCUI_API int css_register_valdef_alias(const char *alias,
				       const char *definitons);

LCUI_API const css_valdef_t *css_resolve_valdef_alias(const char *alias);

LCUI_API css_valdef_t *css_compile_valdef(const char *definition_str);

LCUI_API int css_parse_value(const css_valdef_t *valdef, const char *str,
			     css_style_value_t *val);

LCUI_END_HEADER

#endif
