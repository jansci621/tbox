/*!The Tiny Box Library
 * 
 * TBox is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 * 
 * TBox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with TBox; 
 * If not, see <a href="http://www.gnu.org/licenses/"> http://www.gnu.org/licenses/</a>
 * 
 * Copyright (C) 2009 - 2011, ruki All rights reserved.
 *
 * \author		ruki
 * \file		dlist.c
 *
 */
/* /////////////////////////////////////////////////////////
 * includes
 */
#include "dlist.h"
#include "../libc/libc.h"
#include "../math/math.h"
#include "../platform/platform.h"

/* /////////////////////////////////////////////////////////
 * details
 */

static tb_void_t tb_dlist_item_free(tb_void_t* item, tb_void_t* priv)
{
	tb_dlist_t* dlist = priv;
	if (dlist && dlist->func.free)
	{
		dlist->func.free((tb_byte_t*)item + 8, dlist->func.priv);
	}
}

/* /////////////////////////////////////////////////////////
 * interfaces
 */

tb_dlist_t* tb_dlist_init(tb_size_t step, tb_size_t grow, tb_dlist_item_func_t const* func)
{
	tb_dlist_t* dlist = (tb_dlist_t*)tb_calloc(1, sizeof(tb_dlist_t));
	tb_assert_and_check_return_val(dlist, TB_NULL);

	// init dlist
	dlist->head = 0;
	dlist->last = 0;
	dlist->step = step;
	if (func) dlist->func = *func;

	// init pool, step = next + prev + data
	tb_fpool_item_func_t pool_func;
	pool_func.free = tb_dlist_item_free;
	pool_func.priv = dlist;
	dlist->pool = tb_fpool_init(8 + step, grow, grow, &pool_func);
	tb_assert_and_check_goto(dlist->pool, fail);

	return dlist;
fail:
	if (dlist) tb_dlist_exit(dlist);
	return TB_NULL;
}

tb_void_t tb_dlist_exit(tb_dlist_t* dlist)
{
	if (dlist)
	{
		// clear data
		tb_dlist_clear(dlist);

		// free pool
		if (dlist->pool) tb_fpool_exit(dlist->pool);

		// free it
		tb_free(dlist);
	}
}
tb_void_t tb_dlist_clear(tb_dlist_t* dlist)
{
	if (dlist) 
	{
		// clear pool
		if (dlist->pool) tb_fpool_clear(dlist->pool);

		// reset it
		dlist->head = 0;
		dlist->last = 0;
	}
}
tb_void_t* tb_dlist_itor_at(tb_dlist_t* dlist, tb_size_t itor)
{
	return (tb_void_t*)tb_dlist_itor_const_at(dlist, itor);
}
tb_void_t* tb_dlist_at_head(tb_dlist_t* dlist)
{
	return tb_dlist_itor_at(dlist, tb_dlist_itor_head(dlist));
}
tb_void_t* tb_dlist_at_last(tb_dlist_t* dlist)
{
	return tb_dlist_itor_at(dlist, tb_dlist_itor_last(dlist));
}
tb_void_t const* tb_dlist_itor_const_at(tb_dlist_t const* dlist, tb_size_t itor)
{
	tb_assert_abort(dlist && dlist->pool);
	tb_byte_t const* data = tb_fpool_itor_const_at(dlist->pool, itor);
	tb_assert_abort(data);
	return (data + 8);
}
tb_void_t const* tb_dlist_const_at_head(tb_dlist_t const* dlist)
{
	return tb_dlist_itor_const_at(dlist, tb_dlist_itor_head(dlist));
}
tb_void_t const* tb_dlist_const_at_last(tb_dlist_t const* dlist)
{
	return tb_dlist_itor_const_at(dlist, tb_dlist_itor_last(dlist));
}
tb_size_t tb_dlist_itor_head(tb_dlist_t const* dlist)
{
	tb_assert_abort(dlist);
	return dlist->head;
}
tb_size_t tb_dlist_itor_last(tb_dlist_t const* dlist)
{
	tb_assert_abort(dlist);
	return dlist->last;
}
tb_size_t tb_dlist_itor_tail(tb_dlist_t const* dlist)
{
	tb_assert_abort(dlist);
	return 0;
}
tb_size_t tb_dlist_itor_next(tb_dlist_t const* dlist, tb_size_t itor)
{
	tb_assert_abort(dlist && dlist->pool);

	if (!itor) return dlist->head;
	else
	{
		tb_byte_t const* data = tb_fpool_itor_const_at(dlist->pool, itor);
		tb_assert_abort(data);
		return tb_bits_get_u32_ne(data);
	}
}
tb_size_t tb_dlist_itor_prev(tb_dlist_t const* dlist, tb_size_t itor)
{
	tb_assert_abort(dlist && dlist->pool);

	if (!itor) return dlist->last;
	else
	{
		tb_byte_t const* data = tb_fpool_itor_const_at(dlist->pool, itor);
		tb_assert_abort(data);
		data += 4;
		return tb_bits_get_u32_ne(data);
	}
}
tb_size_t tb_dlist_size(tb_dlist_t const* dlist)
{
	tb_assert_and_check_return_val(dlist && dlist->pool, 0);
	return dlist->pool->size;
}
tb_size_t tb_dlist_maxn(tb_dlist_t const* dlist)
{
	tb_assert_and_check_return_val(dlist && dlist->pool, 0);
	return dlist->pool->maxn;
}
tb_size_t tb_dlist_insert(tb_dlist_t* dlist, tb_size_t index, tb_void_t const* item)
{
	tb_assert_and_check_return_val(dlist && dlist->pool, 0);

	// alloc a new node
	tb_size_t node = tb_fpool_put(dlist->pool, TB_NULL);
	tb_assert_and_check_return_val(node, 0);

	// get the node data
	tb_byte_t* pnode = tb_fpool_itor_at(dlist->pool, node);
	tb_assert_abort(pnode);

	// init node, node <=> 0
	tb_bits_set_u32_ne(pnode, 0);
	tb_bits_set_u32_ne(pnode + 4, 0);

	// copy the item data
	if (item) tb_memcpy(pnode + 8, item, dlist->step);
	else tb_memset(pnode + 8, 0, dlist->step);

	// is null?
	if (!dlist->head && !dlist->last)
	{
		/* dlist: 0 => node => 0
		 *       tail  head   tail
		 *             last
		 */
		dlist->head = node;
		dlist->last = node;
	}
	else
	{
		tb_assert_abort(dlist->head && dlist->last);

		// insert to tail
		if (!index)
		{
			// the last node
			tb_size_t last = dlist->last;
		
			// the last data
			tb_byte_t* plast = tb_fpool_itor_at(dlist->pool, last);
			tb_assert_abort(plast);

			// last <=> node <=> 0
			tb_bits_set_u32_ne(plast, node);
			tb_bits_set_u32_ne(pnode + 4, last);

			// update the last node
			dlist->last = node;
		}
		// insert to head
		else if (index == dlist->head)
		{
			// the head node
			tb_size_t head = dlist->head;
		
			// the head data
			tb_byte_t* phead = tb_fpool_itor_at(dlist->pool, head);
			tb_assert_abort(phead);

			// 0 <=> node <=> head
			tb_bits_set_u32_ne(phead + 4, node);
			tb_bits_set_u32_ne(pnode, head);

			// update the head node
			dlist->head = node;
		}
		// insert to body
		else
		{
			// the body node
			tb_size_t body = index;
		
			// the body data
			tb_byte_t* pbody = tb_fpool_itor_at(dlist->pool, body);
			tb_assert_abort(pbody);

			// the prev node 
			tb_size_t prev = tb_bits_get_u32_ne(pbody + 4);

			// the prev data
			tb_byte_t* pprev = tb_fpool_itor_at(dlist->pool, prev);
			tb_assert_abort(pprev);

			/* 0 <=> ... <=> prev <=> body <=> ... <=> 0
			 * 0 <=> ... <=> prev <=> node <=> body <=> ... <=> 0
			 */
			tb_bits_set_u32_ne(pnode, body);
			tb_bits_set_u32_ne(pnode + 4, prev);
			tb_bits_set_u32_ne(pprev, node);
			tb_bits_set_u32_ne(pbody + 4, node);
		}
	}

	// return the new node
	return node;
}

tb_size_t tb_dlist_insert_head(tb_dlist_t* dlist, tb_void_t const* item)
{
	return tb_dlist_insert(dlist, tb_dlist_itor_head(dlist), item);
}
tb_size_t tb_dlist_insert_tail(tb_dlist_t* dlist, tb_void_t const* item)
{
	return tb_dlist_insert(dlist, tb_dlist_itor_tail(dlist), item);
}
tb_size_t tb_dlist_ninsert(tb_dlist_t* dlist, tb_size_t index, tb_void_t const* item, tb_size_t size)
{
	tb_assert_and_check_return_val(dlist && size, 0);

	// insert items
	tb_size_t node = index;
	while (size--) node = tb_dlist_insert(dlist, node, item);

	// return the first index
	return node;
}
tb_size_t tb_dlist_ninsert_head(tb_dlist_t* dlist, tb_void_t const* item, tb_size_t size)
{
	return tb_dlist_ninsert(dlist, tb_dlist_itor_head(dlist), item, size);
}
tb_size_t tb_dlist_ninsert_tail(tb_dlist_t* dlist, tb_void_t const* item, tb_size_t size)
{
	return tb_dlist_ninsert(dlist, tb_dlist_itor_tail(dlist), item, size);
}
tb_size_t tb_dlist_replace(tb_dlist_t* dlist, tb_size_t index, tb_void_t const* item)
{
	tb_byte_t* data = tb_dlist_itor_at(dlist, index);
	tb_assert_and_check_return_val(data && item, index);
	
	// do free
	if (dlist->func.free) dlist->func.free(data, dlist->func.priv);

	// copy data
	tb_memcpy(data, item, dlist->step);
	return index;
}
tb_size_t tb_dlist_replace_head(tb_dlist_t* dlist, tb_void_t const* item)
{
	return tb_dlist_replace(dlist, tb_dlist_itor_head(dlist), item);
}
tb_size_t tb_dlist_replace_last(tb_dlist_t* dlist, tb_void_t const* item)
{
	return tb_dlist_replace(dlist, tb_dlist_itor_last(dlist), item);
}
tb_size_t tb_dlist_nreplace(tb_dlist_t* dlist, tb_size_t index, tb_void_t const* item, tb_size_t size)
{
	tb_assert_and_check_return_val(dlist && item && size, index);

	tb_size_t itor = index;
	tb_size_t tail = tb_dlist_itor_tail(dlist);
	for (; size-- && itor != tail; itor = tb_dlist_itor_next(dlist, itor)) 
		tb_dlist_replace(dlist, itor, item);
	return index;
}
tb_size_t tb_dlist_nreplace_head(tb_dlist_t* dlist, tb_void_t const* item, tb_size_t size)
{
	return tb_dlist_nreplace(dlist, tb_dlist_itor_head(dlist), item, size);
}
tb_size_t tb_dlist_nreplace_last(tb_dlist_t* dlist, tb_void_t const* item, tb_size_t size)
{
	tb_size_t node = 0;
	tb_size_t itor = tb_dlist_itor_last(dlist);
	tb_size_t tail = tb_dlist_itor_tail(dlist);
	for (; size-- && itor != tail; itor = tb_dlist_itor_prev(dlist, itor)) 
		node = tb_dlist_replace(dlist, itor, item);

	return node;
}
tb_size_t tb_dlist_remove(tb_dlist_t* dlist, tb_size_t index)
{
	tb_assert_and_check_return_val(dlist && dlist->pool && index, index);

	// not null?
	tb_size_t node = index;
	if (dlist->head && dlist->last)
	{
		// only one?
		if (dlist->head == dlist->last)
		{
			tb_assert_abort(dlist->head == index);
			dlist->head = 0;
			dlist->last = 0;
		}
		else
		{
			// remove head?
			if (index == dlist->head)
			{
				// the next node
				tb_size_t next = tb_dlist_itor_next(dlist, index);

				// the next data
				tb_byte_t* pnext = tb_fpool_itor_at(dlist->pool, next);
				tb_assert_abort(pnext);

				/* 0 <=> node <=> next <=> ... <=> 0
				 * 0 <=> next <=> ... <=> 0
				 */
				dlist->head = next;
				tb_bits_set_u32_ne(pnext + 4, 0);

				// update node 
				node = next;
			}
			// remove last?
			else if (index == dlist->last)
			{
				// the prev node
				tb_size_t prev = tb_dlist_itor_prev(dlist, index);

				// the prev data
				tb_byte_t* pprev = tb_fpool_itor_at(dlist->pool, prev);
				tb_assert_abort(pprev);

				/* 0 <=> ... <=> prev <=> node <=> 0
				 * 0 <=> ... <=> prev <=> 0
				 */
				tb_bits_set_u32_ne(pprev, 0);
				dlist->last = prev;

				// update node
				node = prev;
			}
			// remove body?
			else
			{
				// the body node
				tb_size_t body = index;
	
				// the body data
				tb_byte_t* pbody = tb_fpool_itor_at(dlist->pool, body);
				tb_assert_abort(pbody);

				// the next node
				tb_size_t next = tb_bits_get_u32_ne(pbody);

				// the next data
				tb_byte_t* pnext = tb_fpool_itor_at(dlist->pool, next);
				tb_assert_abort(pnext);

				// the prev node
				tb_size_t prev = tb_bits_get_u32_ne(pbody + 4);

				// the prev data
				tb_byte_t* pprev = tb_fpool_itor_at(dlist->pool, prev);
				tb_assert_abort(pprev);

				/* 0 <=> ... <=> prev <=> body <=> next <=> ... <=> 0
				 * 0 <=> ... <=> prev <=> next <=> ... <=> 0
				 */
				tb_bits_set_u32_ne(pprev, next);
				tb_bits_set_u32_ne(pnext + 4, prev);

				// update node
				node = next;
			}
		}

		// free node
		tb_fpool_del(dlist->pool, index);
	}

	return node;
}
tb_size_t tb_dlist_remove_head(tb_dlist_t* dlist)
{
	return tb_dlist_remove(dlist, tb_dlist_itor_head(dlist));
}
tb_size_t tb_dlist_remove_last(tb_dlist_t* dlist)
{
	return tb_dlist_remove(dlist, tb_dlist_itor_last(dlist));
}
tb_size_t tb_dlist_nremove(tb_dlist_t* dlist, tb_size_t index, tb_size_t size)
{
	tb_assert_and_check_return_val(dlist && size, index);

	tb_size_t next = index;
	while (size--) next = tb_dlist_remove(dlist, next);
	return next;
}
tb_size_t tb_dlist_nremove_head(tb_dlist_t* dlist, tb_size_t size)
{
	while (size-- && tb_dlist_size(dlist)) tb_dlist_remove_head(dlist);
	return tb_dlist_itor_head(dlist);
}
tb_size_t tb_dlist_nremove_last(tb_dlist_t* dlist, tb_size_t size)
{
	while (size-- && tb_dlist_size(dlist)) tb_dlist_remove_last(dlist);
	return tb_dlist_itor_last(dlist);
}
