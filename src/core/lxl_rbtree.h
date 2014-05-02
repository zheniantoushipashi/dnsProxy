
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_RBTREE_H_INCLUDE
#define LXL_RBTREE_H_INCLUDE


#include <lxl_config.h>


typedef lxl_uint_t lxl_rbtree_key_t;
typedef struct lxl_rbtree_node_s lxl_rbtree_node_t;

struct lxl_rbtree_node_s {
	unsigned char color;
	lxl_uint_t key;
	lxl_rbtree_node_t *parent;
	lxl_rbtree_node_t *left;
	lxl_rbtree_node_t *right;
};

typedef void (*lxl_rbtree_insert_pt) (lxl_rbtree_node_t *root, lxl_rbtree_node_t *node, lxl_rbtree_node_t *sentinel);

typedef struct {
	lxl_rbtree_node_t *root;
	lxl_rbtree_node_t *sentinel;
	lxl_rbtree_insert_pt insert;
} lxl_rbtree_t;


/*lxl_rbtree_t * lxl_rbtree_create(lxl_rbtree_node_t *sentinel, lxl_rbtree_insert_pt insert);*/

#define lxl_rbtree_init(tree, s, i)		\
	lxl_rbt_black(s);					\
	(tree)->root = s;					\
	(tree)->sentinel = s;				\
	(tree)->insert = i


void lxl_rbtree_insert(lxl_rbtree_t *tree, lxl_rbtree_node_t *node);
void lxl_rbtree_insert_value(lxl_rbtree_node_t *root, lxl_rbtree_node_t *node, lxl_rbtree_node_t *sentinel);
void lxl_rbtree_delete(lxl_rbtree_t *tree, lxl_rbtree_node_t *node);


#define lxl_rbt_red(node)				((node)->color = 1)
#define lxl_rbt_black(node)				((node)->color = 0)
#define lxl_rbt_is_red(node)			((node)->color)
#define lxl_rbt_is_black(node)			(!lxl_rbt_is_red(node))
#define lxl_rbt_copy_color(n1, n2)  	(n1->color = n2->color)


static inline lxl_rbtree_node_t *
lxl_rbtree_min(lxl_rbtree_node_t *node, lxl_rbtree_node_t *sentinel)
{
	while (node->left != sentinel) {
		node = node->left;
	}

	return node;
}

static inline lxl_rbtree_node_t *
lxl_rbtree_max(lxl_rbtree_node_t *node, lxl_rbtree_node_t *sentinel)
{
	while (node->right != sentinel) {
		node = node->right;
	}

	return node;
}

/*static inline lxl_rbtree_node_t *
lxl_rbtree_search(lxl_rbtree_t *tree, lxl_uint_t key)
{
	lxl_rbtree_node_t *node, *sentinel;

	node = tree->root;
	sentinel = tree->sentinel;
	while (node != sentinel) {
		if (node->key > key) {
			node = node->left;
		} else if (node->key < key) {
			node = node->right;
		} else {
			return node;
		}
	}
	
	return NULL;
}*/

#endif	/* LXL_RBTREE_H_INCLUDE */
