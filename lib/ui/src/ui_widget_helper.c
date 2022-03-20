#include <assert.h>
#include <LCUI.h>
#include "../include/ui.h"
#include "internal.h"

void ui_widget_set_padding(ui_widget_t* w, float top, float right, float bottom,
			   float left)
{
	ui_widget_set_style_unit_value(w, css_key_padding_top, top, "px");
	ui_widget_set_style_unit_value(w, css_key_padding_right, right, "px");
	ui_widget_set_style_unit_value(w, css_key_padding_bottom, bottom, "px");
	ui_widget_set_style_unit_value(w, css_key_padding_left, left, "px");
	ui_widget_update_style(w);
}

void ui_widget_set_margin(ui_widget_t* w, float top, float right, float bottom,
			  float left)
{
	ui_widget_set_style_unit_value(w, css_key_margin_top, top, "px");
	ui_widget_set_style_unit_value(w, css_key_margin_right, right, "px");
	ui_widget_set_style_unit_value(w, css_key_margin_bottom, bottom, "px");
	ui_widget_set_style_unit_value(w, css_key_margin_left, left, "px");
	ui_widget_update_style(w);
}

void ui_widget_set_border_color(ui_widget_t* w, css_color_value_t color)
{
	ui_widget_set_style_color_value(w, css_key_border_top_color, color);
	ui_widget_set_style_color_value(w, css_key_border_right_color, color);
	ui_widget_set_style_color_value(w, css_key_border_bottom_color, color);
	ui_widget_set_style_color_value(w, css_key_border_left_color, color);
	ui_widget_update_style(w);
}

void ui_widget_set_border_width(ui_widget_t* w, float width)
{
	ui_widget_set_style_unit_value(w, css_key_border_top_width, width,
				       "px");
	ui_widget_set_style_unit_value(w, css_key_border_right_width, width,
				       "px");
	ui_widget_set_style_unit_value(w, css_key_border_bottom_width, width,
				       "px");
	ui_widget_set_style_unit_value(w, css_key_border_left_width, width,
				       "px");
	ui_widget_update_style(w);
}

void ui_widget_set_border_style(ui_widget_t* w, int style)
{
	ui_widget_set_style_keyword_value(w, css_key_border_top_style, style);
	ui_widget_set_style_keyword_value(w, css_key_border_right_style, style);
	ui_widget_set_style_keyword_value(w, css_key_border_bottom_style,
					  style);
	ui_widget_set_style_keyword_value(w, css_key_border_left_style, style);
}

void ui_widget_set_border(ui_widget_t* w, float width, int style,
			  css_color_value_t color)
{
	ui_widget_set_border_color(w, color);
	ui_widget_set_border_width(w, width);
	ui_widget_set_border_style(w, style);
}

void ui_widget_set_box_shadow(ui_widget_t* w, float x, float y, float blur,
			      pd_color_t color)
{
	// TODO:
	ui_widget_update_style(w);
}

void ui_widget_move(ui_widget_t* w, float left, float top)
{
	ui_widget_set_style_unit_value(w, css_key_top, top, "px");
	ui_widget_set_style_unit_value(w, css_key_left, left, "px");
	ui_widget_update_style(w);
}

void ui_widget_resize(ui_widget_t* w, float width, float height)
{
	ui_widget_set_style_unit_value(w, css_key_width, width, "px");
	ui_widget_set_style_unit_value(w, css_key_height, height, "px");
	ui_widget_update_style(w);
}

void ui_widget_set_visibility(ui_widget_t* w, const char* value)
{
	// TODO
	ui_widget_update_style(w);
}

void ui_widget_show(ui_widget_t* w)
{
	css_style_value_t* s = ui_widget_get_style(w, css_key_display);

	if (s->type == CSS_KEYWORD_VALUE &&
	    s->keyword_value == CSS_KEYWORD_NONE) {
		ui_widget_unset_style(w, css_key_display);
	} else if (!w->computed_style.visible) {
		s = ui_widget_get_matched_style(w, css_key_display);
		if (s->type == CSS_KEYWORD_VALUE &&
		    s->keyword_value != CSS_KEYWORD_NONE) {
			ui_widget_set_style_keyword_value(w, css_key_display,
							  s->keyword_value);
		} else {
			ui_widget_set_style_keyword_value(w, css_key_display,
							  CSS_KEYWORD_BLOCK);
		}
	}
	ui_widget_update_style(w);
}

void ui_widget_hide(ui_widget_t* w)
{
	ui_widget_set_style_keyword_value(w, css_key_display, CSS_KEYWORD_NONE);
	ui_widget_update_style(w);
}

void ui_widget_set_position(ui_widget_t* w, css_keyword_value_t position)
{
	ui_widget_set_style_keyword_value(w, css_key_position, position);
	ui_widget_update_style(w);
}

void ui_widget_set_opacity(ui_widget_t* w, float opacity)
{
	ui_widget_set_style_numberic_value(w, css_key_opacity, opacity);
	ui_widget_update_style(w);
}

void ui_widget_set_box_sizing(ui_widget_t* w, css_keyword_value_t sizing)
{
	ui_widget_set_style_keyword_value(w, css_key_box_sizing, sizing);
	ui_widget_update_style(w);
}

ui_widget_t* ui_widget_get_closest(ui_widget_t* w, const char* type)
{
	ui_widget_t* target;

	for (target = w; target; target = target->parent) {
		if (ui_check_widget_type(target, type)) {
			return target;
		}
	}
	return NULL;
}

static void ui_widget_collect_reference(ui_widget_t* w, void* arg)
{
	const char* ref = ui_widget_get_attribute_value(w, "ref");

	if (ref) {
		dict_add(arg, (void*)ref, w);
	}
}

dict_t* ui_widget_collect_references(ui_widget_t* w)
{
	dict_t* dict;
	static dict_type_t t;

	dict_init_string_key_type(&t);
	dict = dict_create(&t, NULL);
	ui_widget_each(w, ui_widget_collect_reference, dict);
	return dict;
}
