#include "test.h"
#include "ctest.h"
#include "../include/css.h"

void test_css_value(void)
{
	css_valdef_t *valdef;
	css_style_value_t *val;

	valdef = css_compile_valdef("none");
	val = css_parse_value(valdef, "none");
	it_b("parse \"none\"", val->keyword_value, css_get_keyword_key("none"));
}
