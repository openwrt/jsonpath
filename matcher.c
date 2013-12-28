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

#include "matcher.h"

static struct json_object *
jp_match_next(struct jp_opcode *ptr,
              struct json_object *root, struct json_object *cur);

static bool
jp_json_to_op(struct json_object *obj, struct jp_opcode *op)
{
	switch (json_object_get_type(obj))
	{
	case json_type_boolean:
		op->type = T_BOOL;
		op->num = json_object_get_boolean(obj);
		return true;

	case json_type_int:
		op->type = T_NUMBER;
		op->num = json_object_get_int(obj);
		return true;

	case json_type_string:
		op->type = T_STRING;
		op->str = (char *)json_object_get_string(obj);
		return true;

	default:
		return false;
	}
}

static bool
jp_resolve(struct json_object *root, struct json_object *cur,
           struct jp_opcode *op, struct jp_opcode *res)
{
	struct json_object *val;

	switch (op->type)
	{
	case T_THIS:
		val = jp_match(op, cur);

		if (val)
			return jp_json_to_op(val, res);

		return false;

	case T_ROOT:
		val = jp_match(op, root);

		if (val)
			return jp_json_to_op(val, res);

		return false;

	default:
		*res = *op;
		return true;
	}
}

static bool
jp_cmp(struct jp_opcode *op, struct json_object *root, struct json_object *cur)
{
	int delta;
	struct jp_opcode left, right;

	if (!jp_resolve(root, cur, op->down, &left) ||
        !jp_resolve(root, cur, op->down->sibling, &right))
		return false;

	if (left.type != right.type)
		return false;

	switch (left.type)
	{
	case T_BOOL:
	case T_NUMBER:
		delta = left.num - right.num;
		break;

	case T_STRING:
		delta = strcmp(left.str, right.str);
		break;

	default:
		return false;
	}

	switch (op->type)
	{
	case T_EQ:
		return (delta == 0);

	case T_LT:
		return (delta < 0);

	case T_LE:
		return (delta <= 0);

	case T_GT:
		return (delta > 0);

	case T_GE:
		return (delta >= 0);

	case T_NE:
		return (delta != 0);

	default:
		return false;
	}
}

static bool
jp_expr(struct jp_opcode *op, struct json_object *root, struct json_object *cur)
{
	struct jp_opcode *sop;

	switch (op->type)
	{
	case T_WILDCARD:
		return true;

	case T_EQ:
	case T_NE:
	case T_LT:
	case T_LE:
	case T_GT:
	case T_GE:
		return jp_cmp(op, root, cur);

	case T_ROOT:
		return !!jp_match(op, root);

	case T_THIS:
		return !!jp_match(op, cur);

	case T_NOT:
		return !jp_expr(op->down, root, cur);

	case T_AND:
		for (sop = op->down; sop; sop = sop->sibling)
			if (!jp_expr(sop, root, cur))
				return false;
		return true;

	case T_OR:
		for (sop = op->down; sop; sop = sop->sibling)
			if (jp_expr(sop, root, cur))
				return true;
		return false;

	default:
		return false;
	}
}

static struct json_object *
jp_match_expr(struct jp_opcode *ptr,
              struct json_object *root, struct json_object *cur)
{
	int idx, len;
	struct json_object *tmp, *res = NULL;

	switch (json_object_get_type(cur))
	{
	case json_type_object:
		; /* a label can only be part of a statement and a declaration is not a statement */
		json_object_object_foreach(cur, key, val)
		{
			if (!key)
				continue;

			if (jp_expr(ptr, root, val))
			{
				tmp = jp_match_next(ptr->sibling, root, val);

				if (tmp && !res)
					res = tmp;
			}
		}

		break;

	case json_type_array:
		len = json_object_array_length(cur);

		for (idx = 0; idx < len; idx++)
		{
			tmp = json_object_array_get_idx(cur, idx);

			if (jp_expr(ptr, root, tmp))
			{
				tmp = jp_match_next(ptr->sibling, root, tmp);

				if (tmp && !res)
					res = tmp;
			}
		}

		break;

	default:
		break;
	}

	return res;
}

static struct json_object *
jp_match_next(struct jp_opcode *ptr,
              struct json_object *root, struct json_object *cur)
{
	struct json_object *next;

	if (!ptr)
		return cur;

	switch (ptr->type)
	{
	case T_STRING:
	case T_LABEL:
		if (json_object_object_get_ex(cur, ptr->str, &next))
			return jp_match_next(ptr->sibling, root, next);

		break;

	case T_NUMBER:
		next = json_object_array_get_idx(cur, ptr->num);

		if (next)
			return jp_match_next(ptr->sibling, root, next);

		break;

	default:
		return jp_match_expr(ptr, root, cur);
	}

	return NULL;
}

struct json_object *
jp_match(struct jp_opcode *path, json_object *jsobj)
{
	if (path->type == T_LABEL)
		path = path->down;

	return jp_match_next(path->down, jsobj, jsobj);
}
