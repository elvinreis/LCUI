#include <stdio.h>
#include "test.h"
#include "ctest.h"
#include "../include/css.h"

static void test_css_valdef_none(const css_valdef_t *valdef)
{
	int ret;
	css_style_value_t val;

	ret = css_parse_value(valdef, "none", &val) == 0;
	if (ret && val.type == CSS_ARRAY_VALUE) {
		ret = val.array_value[0].keyword_value ==
		      css_get_keyword_key("none");
	}
	it_b("match('none')", ret, 1);
	it_b("notMatch('auto')", css_parse_value(valdef, "auto", &val), -1);
}

static void test_css_valdef_none_or_auto(const css_valdef_t *valdef)
{
	css_style_value_t val;

	css_parse_value(valdef, "none", &val);
	it_b("match('none')", val.keyword_value, css_get_keyword_key("none"));

	css_parse_value(valdef, "auto", &val);
	it_b("match('auto')", val.keyword_value, css_get_keyword_key("auto"));
	it_b("notMatch('normal')", css_parse_value(valdef, "normal", &val), -1);
}

static void test_css_valdef_border(const css_valdef_t *valdef)
{
	int ret;
	css_style_value_t val;

	it_b("match('1px solid #eee')",
	     css_parse_value(valdef, "1px solid #eee", &val), 0);

	it_b("match('#eee 1px solid')",
	     css_parse_value(valdef, "#eee 1px solid", &val), 0);

	it_b("match('solid #eee 1px')",
	     css_parse_value(valdef, "solid #eee 1px", &val), 0);

	ret = css_parse_value(valdef, "1px", &val) == 0;
	if (ret && val.type == CSS_ARRAY_VALUE) {
		ret = val.array_value[0].type == CSS_LENGTH_VALUE &&
		      val.array_value[0].unit_value.value == 1.;
	}
	it_b("match('1px')", ret, 1);

	ret = css_parse_value(valdef, "solid", &val) == 0;
	if (ret && val.type == CSS_ARRAY_VALUE) {
		ret = val.array_value[0].type == CSS_KEYWORD_VALUE &&
		      val.array_value[0].keyword_value ==
			  css_get_keyword_key("solid");
	}
	it_b("match('solid')", ret, 1);

	ret = css_parse_value(valdef, "#eee", &val) == 0;
	if (ret && val.type == CSS_ARRAY_VALUE) {
		ret = ret && val.array_value[0].color_value.r == 238;
		ret = ret && val.array_value[0].color_value.g == 238;
		ret = ret && val.array_value[0].color_value.b == 238;
	}
	it_b("match('#eee')", ret, 1);
}

static void test_css_valdef(const char *definition,
			    void (*func)(const css_valdef_t *))
{
	char str[256] = { 0 };
	const css_valdef_t *valdef;

	snprintf(str, 255, "valdef('%s')", definition);
	test_msg("%s\n", str);
	test_begin();
	valdef = css_compile_valdef(definition);
	if (valdef) {
		func(valdef);
	} else {
		it_b("should be able to parse", 0, 1);
	}
	test_end();
	test_msg("\n");
}

void test_css_value(void)
{
	css_init();

	test_css_valdef("none", test_css_valdef_none);
	test_css_valdef("none | auto", test_css_valdef_none_or_auto);

	css_register_valdef_alias("line-width", "<length>");
	css_register_valdef_alias("line-style", "none | solid");
	test_css_valdef("<line-width> || <line-style> || <color>",
			test_css_valdef_border);
	css_destroy();
}
