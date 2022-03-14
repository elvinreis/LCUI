/* parse.c -- Parse data from string
 *
 * Copyright (c) 2018, Liu chao <lc-soft@live.cn> All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of LCUI nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/css/utils.h"

LCUI_BOOL css_parse_numberic_value(css_style_value_t *s, const char *str)
{
	s->type = CSS_NUMBERIC_VALUE;
	return sscanf(str, "%g", s->numberic_value) == 1;
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
	sscanf(num_str, "%g", &s->unit_value.value);
	strncpy(s->unit_value.unit, p, 4);
	s->unit_value.unit[3] = 0;
	return TRUE;
}

LCUI_BOOL css_parse_rgba(css_style_value_t *val, const char *str)
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

LCUI_BOOL css_parse_rgb(css_style_value_t *val, const char *str)
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

LCUI_BOOL css_parse_color(css_style_value_t *val, const char *str)
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

static LCUI_BOOL css_check_absolute_path(const char *path)
{
	if (path[0] == '/') {
		return TRUE;
	}
	if (isalpha(path[0]) && path[1] == ':' && path[2] == '/') {
		return TRUE;
	}
	return FALSE;
}

LCUI_BOOL css_parse_url(css_style_value_t *s, const char *str, const char *dirname)
{
	size_t n, dirname_len;
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
	if (dirname && !css_check_absolute_path(head)) {
		n += (dirname_len = strlen(dirname));
		s->string_value = malloc((n + 2) * sizeof(char));
		if (!s->string_value) {
			return FALSE;
		}
		strcpy(s->string_value, dirname);
		if (s->string_value[dirname_len - 1] != '/') {
			s->string_value[dirname_len] = '/';
			dirname_len += 1;
			n += 1;
		}
		strncpy(s->string_value + dirname_len,
			head, n - dirname_len);
		s->string_value[n] = 0;
	} else {
		s->string_value = malloc((n + 1) * sizeof(char));
		if (!s->string_value) {
			return FALSE;
		}
		strncpy(s->string_value, head, n);
	}
	s->string_value[n] = 0;
	if (n > 0 && s->string_value[n - 1] == '"') {
		n -= 1;
		s->string_value[n] = 0;
	}
	s->type = CSS_STRING_VALUE;
	return TRUE;
}

LCUI_BOOL css_parse_font_weight(const char *str, int *weight)
{
	int value;
	if (strcmp(str, "normal") == 0) {
		*weight = CSS_FONT_WEIGHT_NORMAL;
		return TRUE;
	}
	if (strcmp(str, "bold") == 0) {
		*weight = CSS_FONT_WEIGHT_BOLD;
		return TRUE;
	}
	if (sscanf(str, "%d", &value) != 1) {
		*weight = CSS_FONT_WEIGHT_NORMAL;
		return FALSE;
	}
	if (value < 100) {
		*weight = CSS_FONT_WEIGHT_THIN;
		return TRUE;
	}
	*weight = y_iround(value / 100.0) * 100;
	return TRUE;
}

LCUI_BOOL css_parse_font_style(const char *str, int *style)
{
	char value[64] = "";
	strtrim(value, str, NULL);
	if (strcmp(value, "normal") == 0) {
		*style = CSS_FONT_STYLE_NORMAL;
	} else if (strcmp(value, "italic") == 0) {
		*style = CSS_FONT_STYLE_ITALIC;
	} else if (strcmp(value, "oblique") == 0) {
		*style = CSS_FONT_STYLE_OBLIQUE;
	} else {
		return FALSE;
	}
	return TRUE;
}
