#include <stdio.h>
#include "../include/css/value.h"
#include "../include/css/utils.h"

LCUI_BOOL css_parse_numberic_value(css_style_value_t *s, const char *str)
{
	s->type = CSS_NUMBERIC_VALUE;
	return sscanf(str, "%lf", &s->numberic_value) == 1;
}

LCUI_BOOL css_parse_unit_value(css_style_value_t *s, const char *str)
{
	int n = 0;
	const char *p;
	char num_str[32];
	LCUI_BOOL has_point = FALSE;

	if (str == NULL) {
		return FALSE;
	}
	/* 先取出数值 */
	for (p = str; *p && n < 30; ++p) {
		if (*p >= '0' && *p <= '9');
		else if (*p == '-' || *p == '+') {
			if (n > 0) {
				n = 0;
				break;
			}
		} else if (*p == '.') {
			if (has_point) {
				n = 0;
				break;
			}
			has_point = TRUE;
		} else {
			break;
		}
		num_str[n++] = *p;
	}
	if (n == 0) {
		return FALSE;
	}
	num_str[n] = 0;
	s->type = CSS_UNIT_VALUE;
	sscanf(num_str, "%lf", &s->unit_value.value);
	strncpy(s->unit_value.unit, p, 4);
	s->unit_value.unit[3] = 0;
	return TRUE;
}

LCUI_BOOL css_parse_string_value(css_style_value_t *val, const char *str)
{
	// TODO
	return FALSE;
}

LCUI_BOOL css_parse_url_value(css_style_value_t *s, const char *str)
{
	size_t n;
	const char *p, *head, *tail;

	p = str;
	tail = head = strstr(p, "url(");
	if (!head) {
		return FALSE;
	}
	while (p) {
		tail = p;
		p = strstr(p + 1, ")");
	}
	if (tail == head) {
		return FALSE;
	}
	head += 4;
	if (*head == '"') {
		++head;
	}
	n = tail - head;
	s->string_value = malloc((n + 1) * sizeof(char));
	if (!s->string_value) {
		return FALSE;
	}
	strncpy(s->string_value, head, n);
	s->string_value[n] = 0;
	if (n > 0 && s->string_value[n - 1] == '"') {
		n -= 1;
		s->string_value[n] = 0;
	}
	s->type = CSS_STRING_VALUE;
	return TRUE;
}

static LCUI_BOOL css_parse_rgba(css_style_value_t *val, const char *str)
{
	float data[4];
	char buf[16];
	const char *p;
	int i, buf_i;

	if (!strstr(str, "rgba(")) {
		return FALSE;
	}
	for (p = str + 5, i = 0, buf_i = 0; *p && i < 4; ++p) {
		if (*p == '.' || (*p >= '0' && *p <= '9')) {
			if (buf_i < 15) {
				buf[buf_i++] = *p;
			}
			continue;
		}
		if (*p == ' ') {
			buf_i = 0;
			continue;
		}
		if (*p == ',' || *p == ')') {
			buf[buf_i] = 0;
			sscanf(buf, "%f", &data[i]);
			buf_i = 0;
			i += 1;
		}
	}
	if (*p) {
		return FALSE;
	}
	val->type = CSS_COLOR_VALUE;
	val->color_value.a = (uchar_t)(255.0 * data[3]);
	val->color_value.r = (uchar_t)data[0];
	val->color_value.g = (uchar_t)data[1];
	val->color_value.b = (uchar_t)data[2];
	return TRUE;
}

static LCUI_BOOL css_parse_rgb(css_style_value_t *val, const char *str)
{
	float data[3];
	char buf[16];
	const char *p;
	int i, buf_i;

	if (!strstr(str, "rgb(")) {
		return FALSE;
	}
	for (p = str + 4, i = 0, buf_i = 0; *p && i < 3; ++p) {
		if (*p == '.' || (*p >= '0' && *p <= '9')) {
			if (buf_i < 15) {
				buf[buf_i++] = *p;
			}
			continue;
		}
		if (*p == ' ') {
			buf_i = 0;
			continue;
		}
		if (*p == ',' || *p == ')') {
			buf[buf_i] = 0;
			sscanf(buf, "%f", &data[i]);
			buf_i = 0;
			i += 1;
		}
	}
	if (*p) {
		return FALSE;
	}
	val->type = CSS_COLOR_VALUE;
	val->color_value.a = 255;
	val->color_value.r = (uchar_t)data[0];
	val->color_value.g = (uchar_t)data[1];
	val->color_value.b = (uchar_t)data[2];
	return TRUE;
}

LCUI_BOOL css_parse_color_value(css_style_value_t *val, const char *str)
{
	const char *p;
	int len = 0, status = 0, r, g, b;
	for (p = str; *p; ++p, ++len) {
		switch (*p) {
		case '#':
			len == 0 ? status = 3 : 0;
			break;
		case 'r':
			status == 0 ? status = 1 : 0;
			break;
		case 'g':
			status == 1 ? status <<= 1 : 0;
			break;
		case 'b':
			status == 2 ? status <<= 1 : 0;
			break;
		case 'a':
			status == 4 ? status <<= 1 : 0;
			break;
		default:
			if (status < 3) {
				break;
			}
		}
	}
	switch (status) {
	case 3:
		status = 0;
		if (len == 4) {
			status = sscanf(str, "#%1X%1X%1X", &r, &g, &b);
			r *= 255 / 0xf; g *= 255 / 0xf; b *= 255 / 0xf;
		} else if (len == 7) {
			status = sscanf(str, "#%2X%2X%2X", &r, &g, &b);
		}
		break;
	case 4: return css_parse_rgb(val, str);
	case 8: return css_parse_rgba(val, str);
	default:break;
	}
	if (status == 3) {
		val->type = CSS_COLOR_VALUE;
		val->color_value.a = 255;
		val->color_value.r = r;
		val->color_value.g = g;
		val->color_value.b = b;
		return TRUE;
	}
	if (strcmp("transparent", str) == 0) {
		val->type = CSS_COLOR_VALUE;
		val->color_value.a = 0;
		val->color_value.r = 255;
		val->color_value.g = 255;
		val->color_value.b = 255;
		return TRUE;
	}
	return FALSE;
}

void css_init_data_types(void)
{
	css_register_value_type("length", css_parse_unit_value);
	css_register_value_type("color", css_parse_color_value);
	css_register_value_type("string", css_parse_string_value);
	css_register_value_type("number", css_parse_numberic_value);
	css_register_value_type("url", css_parse_url_value);
}
