#include "json_c_fuzz.h"
#include "tree.h"
#include "tree_trimming.h"

#include "gtest/gtest.h"
#include "gtest_ext.h"

TEST(TreeTrimmingTest, SubtreeTrimming) {
  srandom(0);  // Fix the random seed

  tree_t *tree = tree_create();
  node_t *start = node_create(START);
  tree->root = start;

  // start -> json
  node_init_subnodes(start, 1);
  node_t *json = node_create(JSON);
  node_set_subnode(start, 0, json);

  // json -> element
  node_init_subnodes(json, 1);
  node_t *element = node_create(ELEMENT);
  node_set_subnode(json, 0, element);

  // element -> ws_1, value, ws_2 (NULL)
  node_init_subnodes(element, 3);
  node_t *ws_1 = node_create(WS);
  node_t *value = node_create(VALUE);
  node_t *ws_2 = node_create(WS);
  node_set_subnode(element, 0, ws_1);
  node_set_subnode(element, 1, value);
  node_set_subnode(element, 2, ws_2);

  // ws_1 -> sp1_1 (" "), ws_3 (NULL)
  node_init_subnodes(ws_1, 2);
  node_t *sp1_1 = node_create_with_val(SP1, " ", 1);
  node_t *ws_3 = node_create(WS);
  node_set_subnode(ws_1, 0, sp1_1);
  node_set_subnode(ws_1, 1, ws_3);

  // value -> object
  node_init_subnodes(value, 1);
  node_t *object = node_create(OBJECT);
  node_set_subnode(value, 0, object);

  // object -> "{", ws_4, "}"
  node_init_subnodes(object, 3);
  node_t *ws_4 = node_create(WS);
  node_set_subnode(object, 0, node_create_with_val(TERM_NODE, "{", 1));
  node_set_subnode(object, 1, ws_4);
  node_set_subnode(object, 2, node_create_with_val(TERM_NODE, "}", 1));

  // ws_4 -> sp1_2 (" "), ws_5 (NULL)
  node_init_subnodes(ws_4, 2);
  node_t *sp1_2 = node_create_with_val(SP1, " ", 1);
  node_t *ws_5 = node_create(WS);
  node_set_subnode(ws_4, 0, sp1_2);
  node_set_subnode(ws_4, 1, ws_5);

  tree_get_size(tree);
  tree_to_buf(tree);
  EXPECT_MEMEQ(" { }", tree->data_buf, tree->data_len);

  // trim `value` subnode
  tree_t *trimmed_tree = subtree_trimming(tree, value);
  tree_to_buf(trimmed_tree);
  EXPECT_TRUE(
      memcmp(" true", trimmed_tree->data_buf, trimmed_tree->data_len) == 0 ||
      memcmp(" false", trimmed_tree->data_buf, trimmed_tree->data_len) == 0 ||
      memcmp(" null", trimmed_tree->data_buf, trimmed_tree->data_len) == 0);
  bool ret = tree_equal(trimmed_tree, tree);
  EXPECT_FALSE(ret);

  tree_free(tree);
  tree_free(trimmed_tree);
}

TEST(TreeTrimmingTest, RecursiveTrimming) {
  srandom(0);  // Fix the random seed

  auto tree = tree_create();
  auto node1 = node_create(1);
  auto node2 = node_create_with_val(0, "{", 1);
  auto node3 = node_create(1);
  auto node4 = node_create_with_val(0, "}", 1);
  auto node5 = node_create_with_val(0, "123", 3);

  node_init_subnodes(node1, 3);
  node_set_subnode(node1, 0, node2);
  node_set_subnode(node1, 2, node4);

  node_init_subnodes(node3, 1);
  node_set_subnode(node3, 0, node5);

  auto _node = node_clone(node1);
  node_set_subnode(node1, 1, _node);
  node_set_subnode(_node, 1, node3);
  tree->root = node1;

  tree_get_size(tree);
  tree_to_buf(tree);
  EXPECT_MEMEQ("{{123}}", tree->data_buf, tree->data_len);

  edge_t  edge = {node1, _node, 1};
  tree_t *trimmed_tree = recursive_trimming(tree, edge);
  tree_to_buf(trimmed_tree);
  EXPECT_MEMEQ("{123}", trimmed_tree->data_buf, trimmed_tree->data_len);
  bool ret = tree_equal(trimmed_tree, tree);
  EXPECT_FALSE(ret);

  tree_free(tree);
  tree_free(trimmed_tree);
}
