/*
 * Copyright (C) 2013 Jo-Philipp Wich <jow@openwrt.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#ifdef JSONC
	#include <json.h>
#else
	#include <json-c/json.h>
#endif

#include "lexer.h"
#include "parser.h"
#include "matcher.h"

static struct json_object *
parse_json(FILE *fd, const char **error)
{
	int len;
	char buf[256];
	struct json_object *obj = NULL;
	struct json_tokener *tok = json_tokener_new();
	enum json_tokener_error err = json_tokener_continue;

	if (!tok)
		return NULL;

	while ((len = fread(buf, 1, sizeof(buf), fd)) > 0)
	{
		obj = json_tokener_parse_ex(tok, buf, len);
		err = json_tokener_get_error(tok);

		if (!err || err != json_tokener_continue)
			break;
	}

	json_tokener_free(tok);

	if (err)
	{
		if (err == json_tokener_continue)
			err = json_tokener_error_parse_eof;

		*error = json_tokener_error_desc(err);
		return NULL;
	}

	return obj;
}

static void
print_string(const char *s)
{
	const char *p;

	printf("'");

	for (p = s; *p; p++)
	{
		if (*p == '\'')
			printf("'\"'\"'");
		else
			printf("%c", *p);
	}

	printf("'");
}

static void
export_value(struct json_object *jsobj, const char *prefix)
{
	int n, len;
	bool first = true;

	if (prefix)
	{
		switch (json_object_get_type(jsobj))
		{
		case json_type_object:
			printf("export %s=", prefix);
			json_object_object_foreach(jsobj, key, val)
			{
				if (!val)
					continue;

				if (!first)
					printf("\\ ");

				print_string(key);
				first = false;
			}
			printf("; ");
			break;

		case json_type_array:
			printf("export %s=", prefix);
			for (n = 0, len = json_object_array_length(jsobj); n < len; n++)
			{
				if (!first)
					printf("\\ ");

				printf("%d", n);
				first = false;
			}
			printf("; ");
			break;

		case json_type_boolean:
			printf("export %s=%d; ", prefix, json_object_get_boolean(jsobj));
			break;

		case json_type_int:
			printf("export %s=%d; ", prefix, json_object_get_int(jsobj));
			break;

		case json_type_double:
			printf("export %s=%f; ", prefix, json_object_get_double(jsobj));
			break;

		case json_type_string:
			printf("export %s=", prefix);
			print_string(json_object_get_string(jsobj));
			printf("; ");
			break;

		case json_type_null:
			break;
		}
	}
	else
	{
		printf("%s\n", json_object_to_json_string(jsobj));
	}
}

static void
export_type(struct json_object *jsobj, const char *prefix)
{
	const char *types[] = {
		"null",
		"boolean",
		"double",
		"int",
		"object",
		"array",
		"string"
	};

	if (prefix)
		printf("export %s=%s; ", prefix, types[json_object_get_type(jsobj)]);
	else
		printf("%s\n", types[json_object_get_type(jsobj)]);
}

static bool
filter_json(int opt, struct json_object *jsobj, char *expr)
{
	struct jp_state *state;
	struct json_object *res = NULL;
	const char *prefix = NULL;

	state = jp_parse(expr);

	if (!state || state->error)
	{
		fprintf(stderr, "In expression '%s': %s\n",
		        expr, state ? state->error : "Out of memory");

		goto out;
	}

	res = jp_match(state->path, jsobj);

	if (res)
	{
		prefix = (state->path->type == T_LABEL) ? state->path->str : NULL;

		switch (opt)
		{
		case 't':
			export_type(res, prefix);
			break;

		default:
			export_value(res, prefix);
			break;
		}
	}

out:
	if (state)
		jp_free(state);

	return !!res;
}

int main(int argc, char **argv)
{
	int opt, rv = 0;
	FILE *input = stdin;
	struct json_object *jsobj = NULL;
	const char *jserr = NULL;

	while ((opt = getopt(argc, argv, "i:e:t:q")) != -1)
	{
		switch (opt)
		{
		case 'i':
			input = fopen(optarg, "r");

			if (!input)
			{
				fprintf(stderr, "Failed to open %s: %s\n",
						optarg, strerror(errno));

				rv = 125;
				goto out;
			}

			break;

		case 't':
		case 'e':
			if (!jsobj)
			{
				jsobj = parse_json(input, &jserr);

				if (!jsobj)
				{
					fprintf(stderr, "Failed to parse json data: %s\n",
					        jserr);

					rv = 126;
					goto out;
				}
			}

			if (!filter_json(opt, jsobj, optarg))
				rv = 1;

			break;

		case 'q':
			fclose(stderr);
			break;
		}
	}

out:
	if (jsobj)
		json_object_put(jsobj);

	if (input != stdin)
		fclose(input);

	return rv;
}
