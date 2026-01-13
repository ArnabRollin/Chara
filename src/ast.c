#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASTNode *create_node(NodeType type, char *val, ASTNode *L, ASTNode *R) {
  ASTNode *node = malloc(sizeof(ASTNode));
  node->type = type;
  node->value = val ? strdup(val) : NULL;
  node->int_value = 0;
  node->left = L;
  node->right = R;
  node->next = NULL;
  return node;
}

void print_ast(ASTNode *node, int depth) {
  if (!node)
    return;

  for (int i = 0; i < depth; i++)
    printf("  ");

  switch (node->type) {
  case NODE_FUNC:
    printf("[Function: %s]\n", node->value);
    break;
  case NODE_CALL:
    printf("[Call: %s]\n", node->value);
    break;
  case NODE_IDENT:
    printf("[Ident: %s]\n", node->value);
    break;
  case NODE_STR:
    printf("[String: %s]\n", node->value);
    break;
  case NODE_NUM:
    printf("[Number: %d]\n", node->int_value);
    break;
  case NODE_PROG:
    printf("[Program]\n");
    break;
  case NODE_VAR:
    printf("[Variable %s = %s]\n", node->value, node->right->value);
    break;
  }

  print_ast(node->left, depth + 1);
  print_ast(node->right, depth + 1);
  print_ast(node->next, depth);
}
