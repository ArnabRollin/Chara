typedef enum {
  NODE_PROG,
  NODE_FUNC,
  NODE_CALL,
  NODE_STR,
  NODE_NUM,
  NODE_IDENT,
  NODE_VAR,
} NodeType;

typedef struct ASTNode {
  NodeType type;
  char *value;
  int int_value;
  struct ASTNode *left;
  struct ASTNode *right;
  struct ASTNode *next;
} ASTNode;

ASTNode *create_node(NodeType type, char *val, ASTNode *L, ASTNode *R);
void print_ast(ASTNode *node, int depth);
