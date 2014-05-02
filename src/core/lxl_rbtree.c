
/*
 * Copyright (C) xianliang.li
 */


#include <string.h>
#include <stdlib.h>

#include "lxl_rbtree.h"


static inline void lxl_rbtree_left_rotate(lxl_rbtree_node_t **root, lxl_rbtree_node_t *sentinel, lxl_rbtree_node_t *node);
static inline void lxl_rbtree_right_rotate(lxl_rbtree_node_t **root, lxl_rbtree_node_t *sentinel, lxl_rbtree_node_t *node);


/*lxl_rbtree_t *
lxl_rbtree_create(lxl_rbtree_node_t *sentinel, lxl_rbtree_insert_pt insert)
{
	lxl_rbtree_t *tree = (lxl_rbtree_t *) malloc(sizeof(lxl_rbtree_t));
	if (!tree) {
		return NULL;
	}

	lxl_rbt_black(sentinel);
	tree->root = sentinel;
	tree->sentinel = sentinel;
	tree->insert = insert;

	return tree;
}*/

void 
lxl_rbtree_insert(lxl_rbtree_t *tree, lxl_rbtree_node_t *node)
{
	lxl_rbtree_node_t **root, *temp, *sentinel;

	root = (lxl_rbtree_node_t **) &tree->root;
	sentinel = tree->sentinel;
	if (*root == sentinel) {
		node->parent = NULL;
		node->left = sentinel;
		node->right = sentinel;
		lxl_rbt_black(node);
		*root = node;

		return;
	}

	tree->insert(*root, node, sentinel);
	
	while (node != *root && lxl_rbt_is_red(node->parent)) {
		if (node->parent == node->parent->parent->left) {
			temp = node->parent->parent->right;
			
			if (lxl_rbt_is_red(temp)) {
				lxl_rbt_black(temp);
				lxl_rbt_black(node->parent);
				lxl_rbt_red(node->parent->parent);
				node = node->parent->parent;
			} else {
				if (node == node->parent->right) {
					node = node->parent;
					lxl_rbtree_left_rotate(root, sentinel, node);	/* node location chanage */
				}

				lxl_rbt_black(node->parent);
				lxl_rbt_red(node->parent->parent);
				lxl_rbtree_right_rotate(root, sentinel, node->parent->parent);
			}
		} else {
			temp = node->parent->parent->left;
			
			if (lxl_rbt_is_red(temp)) {
				lxl_rbt_black(temp);
				lxl_rbt_black(node->parent);
				lxl_rbt_red(node->parent->parent);
				node = node->parent->parent;
			} else {
				if (node == node->parent->left) {
					node = node->parent;
					lxl_rbtree_right_rotate(root, sentinel, node);
				}

				lxl_rbt_black(node->parent);
				lxl_rbt_red(node->parent->parent);
				lxl_rbtree_left_rotate(root, sentinel, node->parent->parent);
			}
		}
	}
	lxl_rbt_black(*root);
}

void
lxl_rbtree_insert_value(lxl_rbtree_node_t *temp, lxl_rbtree_node_t *node, lxl_rbtree_node_t *sentinel)
{
	lxl_rbtree_node_t **p;

	for (; ;) {
		p = (node->key < temp->key) ? &temp->left : &temp->right;
		if (*p == sentinel) {
			break;
		}
		temp = *p;
	}	

	*p = node;
	node->parent = temp;
	node->left = sentinel;
	node->right = sentinel;
	lxl_rbt_red(node);
}

void 
lxl_rbtree_delete(lxl_rbtree_t *tree, lxl_rbtree_node_t *node)
{
	unsigned char red;	
	lxl_rbtree_node_t **root, *sentinel, *subst, *temp, *w;
	
	root = (lxl_rbtree_node_t **) &tree->root;
	sentinel = tree->sentinel;
	if (node->left == sentinel) {
		subst = node;
		temp = node->right;
	} else if (node->right == sentinel) {
		subst = node;
		temp = node->left;
	} else {
		subst = lxl_rbtree_min(node->right, sentinel);
		/*if (subst->left != sentinel) {
			temp = subst->left;
		} else {
			temp = subst->right;
		}*/
		temp = subst->right;
	}

	if (subst == *root) {
		*root = temp;
		(*root)->parent = NULL;	/* lxl add geng jia fuhe */
		lxl_rbt_black(temp);
		
		/*node->parent = NULL;
		node->left = NULL;
		node->right = NULL;
		node->key = 0;*/

		return;
	}

	red = lxl_rbt_is_red(subst);
	if (subst == subst->parent->left) {
		subst->parent->left = temp;
	} else {
		subst->parent->right = temp;
	}
	if (subst == node) {
		temp->parent = subst->parent;
	} else {
		if (subst->parent == node) {
			temp->parent = subst;
		} else {
			temp->parent = subst->parent;
		}
		
		subst->parent = node->parent;
		subst->left = node->left;
		subst->right = node->right;
		lxl_rbt_copy_color(subst, node);
		if (node == *root) {
			*root = subst;
		} else {
			if (node == node->parent->left) {
				node->parent->left = subst;
			} else {
				node->parent->right = subst;
			}
		}
		if (subst->left != sentinel) {
			subst->left->parent = subst;
		}
		if (subst->right != sentinel) {
			subst->right->parent = subst;
		}
	}
	/*
	node->parent = NULL;
	node->left = NULL;
	node->parent = NULL;
	node->key = 0;*/
	if (red) {
		return;
	}

	while (temp != *root && lxl_rbt_is_black(temp)) {
		if (temp == temp->parent->left) {
			w = temp->parent->right;

			if (lxl_rbt_is_red(w)) {
				lxl_rbt_black(w);
				lxl_rbt_red(temp->parent);
				lxl_rbtree_left_rotate(root, sentinel, temp->parent);
				w = temp->parent->right;
			}
				
			if (lxl_rbt_is_black(w->left) && lxl_rbt_is_black(w->right)) {
				lxl_rbt_red(w);
				temp = temp->parent;
			} else {
				if (lxl_rbt_is_black(w->right)) {
					lxl_rbt_black(w->left);
					lxl_rbt_red(w);
					lxl_rbtree_right_rotate(root, sentinel, w);
					w = temp->parent->right;
				}
				
				lxl_rbt_copy_color(w, temp->parent);
				lxl_rbt_black(temp->parent);
				lxl_rbt_black(w->right);
				lxl_rbtree_left_rotate(root, sentinel, temp->parent);
				temp = *root;
			}
		} else {
			w = temp->parent->left;

			if (lxl_rbt_is_red(w)) {
				lxl_rbt_black(w);
				lxl_rbt_red(temp->parent);
				lxl_rbtree_right_rotate(root, sentinel, temp->parent);
				w = temp->parent->left;
			}

			if (lxl_rbt_is_black(w->left) && lxl_rbt_is_black(w->right)) {
				lxl_rbt_red(w);
				temp = temp->parent;
			} else {
				if (lxl_rbt_is_black(w->left)) {
					lxl_rbt_black(w->right);
					lxl_rbt_red(w);
					lxl_rbtree_left_rotate(root, sentinel, w);
					w = temp->parent->left;
				}

				lxl_rbt_copy_color(w, temp->parent);
				lxl_rbt_black(temp->parent);
				lxl_rbt_black(w->left);
				lxl_rbtree_right_rotate(root, sentinel, temp->parent);
				temp = *root;
			}
		}
	}
	lxl_rbt_black(temp);
}

static inline void 
lxl_rbtree_left_rotate(lxl_rbtree_node_t **root, lxl_rbtree_node_t *sentinel, lxl_rbtree_node_t *node)
{
	lxl_rbtree_node_t *temp;

	temp = node->right;
	node->right = temp->left;
	if (temp->left != sentinel) {
		temp->left->parent = node;
	}
	
	temp->parent = node->parent;
	if (node == *root) {
		*root = temp;
	} else if (node == node->parent->left) {
		node->parent->left = temp;
	} else {
		node->parent->right = temp;
	}
	temp->left = node;
	node->parent = temp;
}

static inline void
lxl_rbtree_right_rotate(lxl_rbtree_node_t **root, lxl_rbtree_node_t *sentinel, lxl_rbtree_node_t *node)
{
	lxl_rbtree_node_t *temp;
	
	temp = node->left;
	node->left = temp->right;
	if (temp->right != sentinel) {
		temp->right->parent = node;
	}

	temp->parent = node->parent;
	if (node == *root) {
		*root = temp;
	} else if (node == node->parent->left) {
		node->parent->left = temp;
	} else {
		node->parent->right = temp;
	}
	temp->right = node;
	node->parent = temp;
}


#if 0
#include <stdio.h>
#include <stdlib.h>

#include "lxl_stack.h"
#include "lxl_queue.h"


void lxl_rbtree_order_in(lxl_rbtree_t *tree);
void lxl_rbtree_order_layer(lxl_rbtree_t *tree);


int main(int argc, char *argv[])
{
	int i;
	int array_key[] = {12, 1, 9, 2, 0, 11, 7, 19, 4, 15, 18, 5, 14, 13, 10, 16, 6, 3, 8, 17};
	lxl_rbtree_node_t sentinel, *node;
//	lxl_rbtree_t *tree = lxl_rbtree_create(&sentinel, lxl_rbtree_insert_value);

	lxl_rbtree_t tree;
	lxl_rbtree_init(&tree, &sentinel, lxl_rbtree_insert_value);
	for (i = 0; i < 20; ++i) {
		node = (lxl_rbtree_node_t *) malloc(sizeof(lxl_rbtree_node_t));
		node->key = array_key[i];
		lxl_rbtree_insert(&tree, node);
	}
	lxl_rbtree_order_in(&tree);
	fprintf(stderr, "==========================================================\n");
	lxl_rbtree_order_layer(&tree);
	for (i = 0; i < 20; ++i) {
		node = lxl_rbtree_search(&tree, array_key[i]);
		if (node) {
			fprintf(stderr, "===========delete %lu ========\n", node->key);
			lxl_rbtree_delete(&tree, node);
			free(node);
		}
		lxl_rbtree_order_layer(&tree);
	}

	/*for (i = 0; i < 50; ++i) {
		node = (lxl_rbtree_node_t *) malloc(sizeof(lxl_rbtree_node_t));
		node->key = i;
		lxl_rbtree_insert(&tree, node);
	}
	lxl_rbtree_order_layer(&tree);*/

	return 0;
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
#endif
