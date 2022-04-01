
#ifndef LIBCSS_INCLUDE_CSS_KEYWORDS_H
#define LIBCSS_INCLUDE_CSS_KEYWORDS_H

#include <LCUI/header.h>
#include "def.h"

LCUI_BEGIN_HEADER

LCUI_API int css_register_keyword(int key, const char *name);
LCUI_API int css_get_keyword_key(const char *str);
LCUI_API const char *css_get_keyword_name(int val);
LCUI_API void css_init_keywords(void);
LCUI_API void css_destroy_keywords(void);

LCUI_END_HEADER

#endif
