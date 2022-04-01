#include "../include/css/properties.h"
#include "../include/css/value.h"
#include "../include/css/style_value.h"

static struct css_properties_module_t {
	/**
	 * 样式属性列表
	 * css_property_definition_t*[]
	 */
	css_property_definition_t **list;
	unsigned length;

	/**
	 * 样式属性表，以名称索引
	 * dict_t<string, css_property_definition_t>
	 */
	dict_t *map;
} css_properties;

static void css_property_definition_destroy(css_property_definition_t *prop)
{
	if (prop->name) {
		free(prop->name);
		prop->name = NULL;
	}
	free(prop);
}

static int css_register_property_with_key(unsigned key, const char *name,
					  const char *syntax,
					  const char *initial_value)
{
	css_property_definition_t *prop;
	css_property_definition_t **props;

	if (key >= css_properties.length) {
		props = realloc(css_properties.list,
				key * sizeof(css_property_definition_t *));
		if (!props) {
			return -1;
		}
		css_properties.list = props;
		css_properties.length = key;
	}
	prop = malloc(sizeof(css_property_definition_t));
	if (prop) {
		return -1;
	}
	// TODO
	// if (css_compile_syntax(syntax, &prop->syntax) != 0) {
	// 	css_property_definition_destroy(prop);
	// 	return -2;
	// }
	// css_parse_style_value_with_syntax(&prop->syntax, initial_value,
	// 				  &prop->initial_value);
	prop->name = strdup2(name);
	prop->key = key;
	props[prop->key] = prop;
	dict_add(css_properties.map, prop->name, prop);
	return prop->key;
}

int css_register_property(const char *name, const char *definition,
			  const char *initial_value)
{
	return css_register_property_with_key((int)css_properties.length, name,
					      definition, initial_value);
}

const css_property_definition_t *css_get_property(const char *name)
{
	return dict_fetch_value(css_properties.map, name);
}

const css_property_definition_t *css_get_property_by_key(int key)
{
	if (key >= 0 && (size_t)key < css_properties.length) {
		return css_properties.list[key];
	}
	return NULL;
}

unsigned css_get_property_count(void)
{
	return css_properties.length;
}

void css_init_properties(void)
{
	static dict_type_t dt = { 0 };

	dict_init_string_key_type(&dt);
	css_properties.map = dict_create(&dt, NULL);
	css_properties.list = NULL;
	css_properties.length = 0;

	css_register_valdef_alias("shadow", "<length>{2,4} && <color>?");
	css_register_valdef_alias(
	    "content-position", "center | start | end | flex-start | flex-end");
	css_register_valdef_alias(
	    "content-distribution",
	    "space-between | space-around | space-evenly | stretch");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/visibility */
	css_register_property_with_key(css_key_visibility, "visibility",
				       "visible | hidden", "visible");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/width */
	css_register_property_with_key(
	    css_key_width, "width", "auto | <length> | <percentage>", "auto");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/height */
	css_register_property_with_key(
	    css_key_height, "height", "auto | <length> | <percentage>", "auto");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/min-width */
	css_register_property_with_key(css_key_min_width, "min-width",
				       "auto | <length> | <percentage>",
				       "auto");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/min-height */
	css_register_property_with_key(css_key_min_height, "min-height",
				       "auto | <length> | <percentage>",
				       "auto");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/max-width */
	css_register_property_with_key(css_key_max_width, "max-width",
				       "auto | <length> | <percentage>",
				       "auto");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/max-height */
	css_register_property_with_key(css_key_max_height, "max-height",
				       "auto | <length> | <percentage>",
				       "auto");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/display */
	css_register_property_with_key(css_key_display, "display",
				       "none | block | inline-block | flex",
				       "block");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/z-index */
	css_register_property_with_key(css_key_z_index, "z-index",
				       "auto | <integer>", "auto");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/top */
	css_register_property_with_key(
	    css_key_top, "top", "<length> | <percentage> | auto", "auto");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSSright */
	css_register_property_with_key(
	    css_key_right, "right", "<length> | <percentage> | auto", "auto");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/left */
	css_register_property_with_key(
	    css_key_left, "left", "<length> | <percentage> | auto", "auto");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/bottom */
	css_register_property_with_key(
	    css_key_bottom, "bottom", "<length> | <percentage> | auto", "auto");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/position */
	css_register_property_with_key(css_key_position, "position",
				       "static | relative | absolute",
				       "static");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/opacity */
	css_register_property_with_key(css_key_opacity, "opacity",
				       "<number> | <percentage>", "1");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/vertical-align
	 */
	css_register_property_with_key(css_key_vertical_align, "vertical-align",
				       "middle | bottom | top", "top");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/background-color */
	css_register_property_with_key(css_key_background_color,
				       "background-color", "<color>",
				       "transparent");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/background-position
	 */
	css_register_property_with_key(css_key_background_position,
				       "background-position", "[\
		[ left | center | right | top | bottom | <length> | <percentage> ]\
		| [ left | center | right | <length> | <percentage> ] [ top | center | bottom | <length> | <percentage> ]\
	]",
				       "0% 0%");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/background-size */
	css_register_property_with_key(
	    css_key_background_size, "background-size",
	    "[ <length> | <percentage> | auto ]{1,2} | cover | contain",
	    "auto auto");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/background-image */
	css_register_property_with_key(css_key_background_image,
				       "background-image", "none | <image>",
				       "none");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/padding-left
	 */
	css_register_property_with_key(css_key_padding_left, "padding-left",
				       "<length> | <percentage>", "0");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/padding-right
	 */
	css_register_property_with_key(css_key_padding_right, "padding-right",
				       "<length> | <percentage>", "0");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/padding-top */
	css_register_property_with_key(css_key_padding_top, "padding-top",
				       "<length> | <percentage>", "0");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/padding-bottom
	 */
	css_register_property_with_key(css_key_padding_bottom, "padding-bottom",
				       "<length> | <percentage>", "0");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/margin-left */
	css_register_property_with_key(css_key_margin_left, "margin-left",
				       "<length> | <percentage>", "0");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/margin-right
	 */
	css_register_property_with_key(css_key_margin_right, "margin-right",
				       "<length> | <percentage>", "0");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/margin-top */
	css_register_property_with_key(css_key_margin_top, "margin-top",
				       "<length> | <percentage>", "0");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/margin-bottom
	 */
	css_register_property_with_key(css_key_margin_bottom, "margin-bottom",
				       "<length> | <percentage>", "0");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-color */
	css_register_property_with_key(css_key_border_top_color,
				       "border-top-color", "<color>",
				       "transparent");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-right-color
	 */
	css_register_property_with_key(css_key_border_right_color,
				       "border-right-color", "<color>",
				       "transparent");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-color
	 */
	css_register_property_with_key(css_key_border_bottom_color,
				       "border-bottom-color", "<color>",
				       "transparent");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-left-color */
	css_register_property_with_key(css_key_border_left_color,
				       "border-left-color", "<color>",
				       "transparent");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-width */
	css_register_property_with_key(css_key_border_top_width,
				       "border-top-width", "<length>", "0");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-right-width
	 */
	css_register_property_with_key(css_key_border_right_width,
				       "border-right-width", "<length>", "0");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-width
	 */
	css_register_property_with_key(css_key_border_bottom_width,
				       "border-bottom-width", "<length>", "0");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-left-width */
	css_register_property_with_key(css_key_border_left_width,
				       "border-left-width", "<length>", "0");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-width */
	css_register_property_with_key(css_key_border_top_width,
				       "border-top-width", "<length>", "0");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-right-width
	 */
	css_register_property_with_key(css_key_border_right_width,
				       "border-right-width", "<length>", "0");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-width
	 */
	css_register_property_with_key(css_key_border_bottom_width,
				       "border-bottom-width", "<length>", "0");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-left-width */
	css_register_property_with_key(css_key_border_left_width,
				       "border-left-width", "<length>", "0");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-style */
	css_register_property_with_key(css_key_border_top_style,
				       "border-top-style", "none | solid",
				       "none");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-right-style
	 */
	css_register_property_with_key(css_key_border_right_style,
				       "border-right-style", "none | solid",
				       "none");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-style
	 */
	css_register_property_with_key(css_key_border_bottom_style,
				       "border-bottom-style", "none | solid",
				       "none");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-left-style */
	css_register_property_with_key(css_key_border_left_style,
				       "border-left-style", "none | solid",
				       "none");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-left-radius
	 */
	css_register_property_with_key(css_key_border_top_left_radius,
				       "border-top-left-radius",
				       "<length> | <percentage>", "0");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-top-right-radius
	 */
	css_register_property_with_key(css_key_border_top_right_radius,
				       "border-top-right-radius",
				       "<length> | <percentage>", "0");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-left-radius
	 */
	css_register_property_with_key(css_key_border_bottom_left_radius,
				       "border-bottom-left-radius",
				       "<length> | <percentage>", "0");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/border-bottom-right-radius
	 */
	css_register_property_with_key(css_key_border_bottom_right_radius,
				       "border-bottom-right-radius",
				       "<length> | <percentage>", "0");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/box-shadow */
	css_register_property_with_key(css_key_box_shadow, "box-shadow",
				       "none | <shadow>", "none");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/pointer-events
	 */
	css_register_property_with_key(css_key_pointer_events, "pointer-events",
				       "auto | none", "auto");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/box-sizing */
	css_register_property_with_key(css_key_box_sizing, "box-sizing",
				       "content-box | border-box",
				       "content-box");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/flex-basis */
	css_register_property_with_key(css_key_flex_basis, "flex-basis",
				       "auto | <width>", "auto");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/flex-direction
	 */
	css_register_property_with_key(css_key_flex_direction, "flex-direction",
				       "row | column", "row");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/flex-grow */
	css_register_property_with_key(css_key_flex_grow, "flex-grow",
				       "<number>", "0");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/flex-shrink */
	css_register_property_with_key(css_key_flex_shrink, "flex-shrink",
				       "<number>", "1");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/flex-wrap */
	css_register_property_with_key(css_key_flex_wrap, "flex-wrap",
				       "nowrap | wrap", "nowrap");

	/** @see
	 * https://developer.mozilla.org/en-US/docs/Web/CSS/justify-content */
	css_register_property_with_key(
	    css_key_justify_content, "justify-content",
	    "normal | <baseline-position> | <content-distribution>", "normal");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/align-content
	 */
	css_register_property_with_key(
	    css_key_align_content, "align-content",
	    "normal | <baseline-position> | <content-distribution>", "normal");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/align-items */
	css_register_property_with_key(css_key_align_items, "align-items",
				       "normal | stretch", "normal");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/color */
	css_register_property_with_key(css_key_color, "color", "<color>",
				       "#000");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/font-family */
	css_register_property_with_key(css_key_font_family, "font-family",
				       "<string>", "");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/font-size */
	css_register_property_with_key(css_key_font_size, "font-size",
				       "<length> | <percentage>", "16px");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/font-style */
	css_register_property_with_key(css_key_font_style, "font-style",
				       "normal | italic | oblique", "normal");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/text-align */
	css_register_property_with_key(css_key_text_align, "text-align",
				       "left | center | right", "left");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/line-height */
	css_register_property_with_key(css_key_line_height, "line-height",
				       "<number> | <length> | <percentage>",
				       "1.6");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/content */
	css_register_property_with_key(css_key_content, "content", "<string>",
				       "");

	/** @see https://developer.mozilla.org/en-US/docs/Web/CSS/white-space */
	css_register_property_with_key(css_key_white_space, "white-space",
				       "normal | nowrap", "");
}

void css_destroy_properties(void)
{
	unsigned i;

	dict_destroy(css_properties.map);
	for (i = 0; i < css_properties.length; ++i) {
		free(css_properties.list[i]->name);
		css_style_value_destroy(&css_properties.list[i]->initial_value);
		css_properties.list[i] = NULL;
	}
	free(css_properties.list);
	css_properties.map = NULL;
	css_properties.list = NULL;
	css_properties.length = 0;
}
