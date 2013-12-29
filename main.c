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
parse_json(FILE *fd)
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

	return err ? NULL : obj;
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
export_json(struct json_object *jsobj, char *expr)
{
	bool first;
	struct jp_state *state;
	struct json_object *res;
	const char *prefix;

	state = jp_parse(expr);

	if (!state || state->error)
	{
		fprintf(stderr, "In expression '%s': %s\n",
		        expr, state ? state->error : "Out of memory");

		goto out;
	}

	res = jp_match(state->path, jsobj);

	if (state->path->type == T_LABEL)
	{
		prefix = state->path->str;

		switch (json_object_get_type(res))
		{
		case json_type_object:
			printf("export %s_TYPE=object; ", prefix);

			first = true;
			printf("export %s_KEYS=", prefix);
			json_object_object_foreach(res, key, val)
			{
				if (!val)
					continue;

				if (!first)
					printf("\\ ");

				print_string(key);
				first = false;
			}
			printf("; ");

			//printf("export %s=", prefix);
			//print_string(json_object_to_json_string(res));
			//printf("; ");

			break;

		case json_type_array:
			printf("export %s_TYPE=array; ", prefix);
			printf("export %s_LENGTH=%d; ",
			       prefix, json_object_array_length(res));

			//printf("export %s=", prefix);
			//print_string(json_object_to_json_string(res));
			//printf("; ");
			break;

		case json_type_boolean:
			printf("export %s_TYPE=bool; ", prefix);
			printf("export %s=%d; ", prefix, json_object_get_boolean(res));
			break;

		case json_type_int:
			printf("export %s_TYPE=int; ", prefix);
			printf("export %s=%d; ", prefix, json_object_get_int(res));
			break;

		case json_type_double:
			printf("export %s_TYPE=double; ", prefix);
			printf("export %s=%f; ", prefix, json_object_get_double(res));
			break;

		case json_type_string:
			printf("export %s_TYPE=string; ", prefix);
			printf("export %s=", prefix);
			print_string(json_object_get_string(res));
			printf("; ");
			break;

		case json_type_null:
			printf("unset %s %s_TYPE %s_LENGTH %s_KEYS; ",
				   prefix, prefix, prefix, prefix);
			break;
		}
	}
	else
	{
		printf("%s\n", json_object_to_json_string(res));
	}

out:
	if (state)
		jp_free(state);
}

int main(int argc, char **argv)
{
	int opt;
	FILE *input = stdin;
	struct json_object *jsobj = NULL;

	while ((opt = getopt(argc, argv, "i:e:")) != -1)
	{
		switch (opt)
		{
		case 'i':
			input = fopen(optarg, "r");

			if (!input)
			{
				fprintf(stderr, "Failed to open %s: %s\n",
						optarg, strerror(errno));

				exit(1);
			}

			break;

		case 'e':
			if (!jsobj)
			{
				jsobj = parse_json(input);

				if (!jsobj)
				{
					fprintf(stderr, "Failed to parse json data\n");
					exit(2);
				}
			}

			export_json(jsobj, optarg);
			break;
		}
	}

	if (jsobj)
		json_object_put(jsobj);

	fclose(input);

	return 0;
}
