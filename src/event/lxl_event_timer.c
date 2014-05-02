
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_event_timer.h>


lxl_rbtree_t 		lxl_event_timer_rbtree;
lxl_rbtree_node_t	lxl_event_timer_sentinel;


lxl_uint_t 
lxl_event_find_timer(void) 
{
	lxl_int_t timer;
	lxl_rbtree_node_t *node, *root, *sentinel;

	if (lxl_event_timer_rbtree.root == &lxl_event_timer_sentinel) {
		return (lxl_uint_t) -1;
	}
	
	root = lxl_event_timer_rbtree.root;
	sentinel = lxl_event_timer_rbtree.sentinel;
	node = lxl_rbtree_min(root, sentinel);
	timer = (lxl_int_t) (node->key - lxl_current_msec);

	/* return (lxl_uint_t) (timer > 0 ? timer : 0); */
	if (timer < 0) {
		return (lxl_uint_t) LXL_TIMER_LAZY_DELAY;
	} 

	return (lxl_uint_t) lxl_max(timer, LXL_TIMER_LAZY_DELAY);
}

void 
lxl_event_expire_timers(void)
{
	lxl_event_t *ev;
	lxl_rbtree_node_t *node, *root, *sentinel;
	
	sentinel = lxl_event_timer_rbtree.sentinel;
	for (; ;) {
		root = lxl_event_timer_rbtree.root;
		if (root == sentinel) {
			return;
		}
		
		node = lxl_rbtree_min(root, sentinel);
		if (node->key <= lxl_current_msec) {
			ev = (lxl_event_t *) ((char *) node - offsetof(lxl_event_t, timer));
			lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "event timer del: %lu", ev->timer.key);
			lxl_rbtree_delete(&lxl_event_timer_rbtree, &ev->timer);

			ev->timer_set = 0;
			ev->timedout = 1;
			ev->handler(ev);
			
			continue;
		}

		break;
	}
}

#if 0
#include <stdio.h>
#include <stdlib.h>

#include "lxl_queue.h"
#include "lxl_stack.h"


void lxl_rbtree_order_layer(lxl_rbtree_t *tree);
void lxl_rbtree_order_in(lxl_rbtree_t *tree);


int main(int argc, char *argv[])
{
	int i;
	lxl_event_t *ev;

	lxl_event_timer_init();
	for (i = 0; i < 1000*1000; ++i) {
		ev = (lxl_event_t *) malloc(sizeof(lxl_event_t));
		ev->timedout = 0;
		ev->timer_set = 0;
		lxl_event_add_timer(ev, (lxl_uint_t)i);
	}
	
	lxl_event_expire_timers();
	//lxl_rbtree_order_layer(&lxl_event_timer_rbtree);
	lxl_rbtree_order_in(&lxl_event_timer_rbtree);

	return 0;
}

void 
lxl_rbtree_order_layer(lxl_rbtree_t *tree)
{
	if (tree->root == tree->sentinel) {
		return;
	}

	lxl_uint_t top;
	lxl_queue_t *q = lxl_queue_create(20);
	lxl_rbtree_node_t *node, *sentinel;
	
	sentinel = tree->sentinel;
	lxl_queue_in(q, tree->root);
	top = q->rear;
	while (!lxl_queue_empty(q)) {
		while (q->front != top) {
			node = lxl_queue_out(q);
			if (lxl_rbt_is_black(node)) {
				fprintf(stderr, "%lu", node->key);
			} else {
				fprintf(stderr, "\033[31;49;1m%lu\033[0m", node->key);
				//fprintf(stderr, "%lu(red)\t", node->key);
			}
			if (node->parent) {
				fprintf(stderr, "(%lu)\t", node->parent->key);
			} else {
				fprintf(stderr, "\t");
			}
			if (node->left != sentinel) {
				lxl_queue_in(q, node->left);
			}
			if (node->right != sentinel) {
				lxl_queue_in(q, node->right);
			}
		}
		top = q->rear;
		fprintf(stderr, "===\n");
	}
}

void
lxl_rbtree_order_in(lxl_rbtree_t *tree)
{
	lxl_stack_t *s = lxl_stack_create(20);
	lxl_rbtree_node_t *node, *sentinel;

	sentinel = tree->sentinel;
	node = tree->root;
	while (node != sentinel || !lxl_stack_empty(s)) {
		while (node != sentinel) {
			lxl_stack_push(s, node);
			node = node->left;
		}
		if (!lxl_stack_empty(s)) {
			node = lxl_stack_pop(s);
			fprintf(stderr, "%lu\t", node->key);
			node = node->right;
		}
	}
	fprintf(stderr, "\n");
}
#endif 
