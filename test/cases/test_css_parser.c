#include <stdio.h>
#include <LCUI.h>
#include <LCUI/ui.h>
#include <LCUI/ui/builder.h>
#include "ctest.h"

static void test_btn_text_style(void)
{
	css_style_value_t *s;

	s = ui_get_widget("test-textview")->style->list;
	it_i("width", (int)s[css_key_width].unit_value.value, 100);
	it_i("height", (int)s[css_key_height].unit_value.value, 60);
	it_i("position", s[css_key_position].keyword_value,
	     CSS_KEYWORD_ABSOLUTE);
	it_i("top", (int)s[css_key_top].unit_value.value, 12);
	it_i("left", (int)s[css_key_left].unit_value.value, 20);
}

static void test_btn_hover_text_style(void)
{
	css_style_value_t *s;

	s = ui_get_widget("test-textview")->style->list;
	it_i("background-color", s[css_key_background_color].color_value.value,
	     0xffff0000);
	it_i("background-size", s[css_key_background_size].keyword_value,
	     CSS_KEYWORD_CONTAIN);
}

static void test_flex_box(void)
{
	css_style_value_t *s;

	s = ui_get_widget("test-flex-box")->style->list;
	it_i("flex-grow", s[css_key_flex_grow].numberic_value, 0);
	it_i("flex-shrink", s[css_key_flex_shrink].numberic_value, 0);
	it_i("flex-basis", s[css_key_flex_basis].keyword_value,
	     CSS_KEYWORD_AUTO);
	it_i("flex-direction", s[css_key_flex_direction].keyword_value,
	     CSS_KEYWORD_COLUMN);
	it_i("flex-wrap", s[css_key_flex_wrap].keyword_value,
	     CSS_KEYWORD_NOWRAP);
	it_i("justify-content", s[css_key_justify_content].keyword_value,
	     CSS_KEYWORD_CENTER);
	it_i("align-items", s[css_key_align_items].keyword_value,
	     CSS_KEYWORD_FLEX_END);
	it_i("align-content", s[css_key_align_content].keyword_value,
	     CSS_KEYWORD_FLEX_END);
}

static void test_parse_flex_initial(void)
{
	css_style_value_t *s;

	s = ui_get_widget("test-flex-initial")->style->list;
	it_i("<flex-grow>", s[css_key_flex_grow].numberic_value, 0);
	it_i("<flex-shrink>", s[css_key_flex_shrink].numberic_value, 1);
	it_i("<flex-basis>", s[css_key_flex_basis].keyword_value,
	     CSS_KEYWORD_AUTO);
}
static void test_parse_flex_auto(void)
{
	css_style_value_t *s;

	s = ui_get_widget("test-flex-auto")->style->list;
	it_i("<flex-grow>", s[css_key_flex_grow].numberic_value, 1);
	it_i("<flex-shrink>", s[css_key_flex_shrink].numberic_value, 1);
	it_i("<flex-basis>", s[css_key_flex_basis].keyword_value,
	     CSS_KEYWORD_AUTO);
}

static void test_parse_flex_none(void)
{
	css_style_value_t *s;

	s = ui_get_widget("test-flex-none")->style->list;
	it_i("<flex-grow>", s[css_key_flex_grow].numberic_value, 0);
	it_i("<flex-shrink>", s[css_key_flex_shrink].numberic_value, 0);
	it_i("<flex-basis>", s[css_key_flex_basis].keyword_value,
	     CSS_KEYWORD_AUTO);
}

static void test_parse_flex_1(void)
{
	css_style_value_t *s;

	s = ui_get_widget("test-flex-1")->style->list;
	it_i("<flex-grow>", s[css_key_flex_grow].numberic_value, 1);
	it_b("<flex-shrink>.isValid?",
	     s[css_key_flex_shrink].type > CSS_INVALID_VALUE, FALSE);
	it_b("<flex-basis>.isValid?",
	     s[css_key_flex_basis].type > CSS_INVALID_VALUE, FALSE);
}

static void test_parse_flex_100px(void)
{
	css_style_value_t *s;

	s = ui_get_widget("test-flex-100px")->style->list;
	it_b("<flex-grow>.isValid?",
	     s[css_key_flex_grow].type > CSS_INVALID_VALUE, FALSE);
	it_b("<flex-shrink>.isValid?",
	     s[css_key_flex_shrink].type > CSS_INVALID_VALUE, FALSE);
	it_i("<flex-basis>", (int)s[css_key_flex_basis].unit_value.value, 100);
}

static void test_parse_flex_1_100px(void)
{
	css_style_value_t *s;

	s = ui_get_widget("test-flex-1-100px")->style->list;
	it_b("<flex-grow>.isValid?",
	     s[css_key_flex_grow].type > CSS_INVALID_VALUE, FALSE);
	it_i("<flex-shrink>", s[css_key_flex_shrink].numberic_value, 1);
	it_i("<flex-basis>", (int)s[css_key_flex_basis].unit_value.value, 100);
}
static void test_parse_flex_0_0_100px(void)
{
	css_style_value_t *s;

	s = ui_get_widget("test-flex-0-0-100px")->style->list;
	it_i("<flex-grow>", s[css_key_flex_grow].numberic_value, 0);
	it_i("<flex-shrink>", s[css_key_flex_shrink].numberic_value, 0);
	it_i("<flex-basis>", (int)s[css_key_flex_basis].unit_value.value, 100);
}

void test_css_parser(void)
{
	ui_widget_t *root, *box, *btn;

	lcui_init();
	box = ui_load_xml_file("test_css_parser.xml");
	it_b("should successfully load test_css_parser.xml", !!box, TRUE);
	if (!box) {
		lcui_destroy();
		return;
	}
	root = ui_root();
	ui_widget_append(root, box);
	ui_widget_unwrap(box);
	ui_update();

	btn = ui_get_widget("test-btn");
	describe(".btn .text", test_btn_text_style);
	ui_widget_add_status(btn, "hover");
	ui_update();

	describe(".btn:hover .text", test_btn_hover_text_style);
	describe("#test-flex-box", test_flex_box);
	describe("parse 'flex: auto;'", test_parse_flex_auto);
	describe("parse 'flex: none;'", test_parse_flex_none);
	describe("parse 'flex: initial;'", test_parse_flex_initial);
	describe("parse 'flex: 1;'", test_parse_flex_1);
	describe("parse 'flex: 100px;'", test_parse_flex_100px);
	describe("parse 'flex: 1 100px;'", test_parse_flex_1_100px);
	describe("parse 'flex: 0 0 100px;'", test_parse_flex_0_0_100px);
	lcui_destroy();
}
