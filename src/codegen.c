#include "ast.h"

#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <stdio.h>
#include <string.h>

#include <llvm-c/Core.h>
#include <stdlib.h>
#include <string.h>

LLVMBasicBlockRef saved_block = NULL;

typedef struct FunctionEntry {
  char *name;
  LLVMValueRef func_ref;
  struct FunctionEntry *next;
  int arity;
} FunctionEntry;

FunctionEntry *function_table = NULL;

typedef struct VarEntry {
  char *name;              // variable name
  LLVMValueRef alloca_ref; // pointer returned by LLVMBuildAlloca
  struct VarEntry *next;
} VarEntry;

static VarEntry *var_table = NULL; // head of linked list

// Add a function to our "Hash Map"
void symtab_insert(const char *name, LLVMValueRef func, int arity) {
  FunctionEntry *entry = malloc(sizeof(FunctionEntry));
  entry->name = strdup(name);
  entry->func_ref = func;
  entry->arity = arity;
  entry->next = function_table;
  function_table = entry;
}

// Lookup a function by name
LLVMValueRef symtab_lookup(const char *name) {
  FunctionEntry *curr = function_table;
  while (curr) {
    if (strcmp(curr->name, name) == 0)
      return curr->func_ref;
    curr = curr->next;
  }
  return NULL;
}

FunctionEntry *symtab_lookup_entry(const char *name) {
  FunctionEntry *curr = function_table;
  while (curr) {
    if (strcmp(curr->name, name) == 0)
      return curr;
    curr = curr->next;
  }
  return NULL;
}

void var_insert(const char *name, LLVMValueRef alloca_ref) {
  VarEntry *entry = malloc(sizeof(VarEntry));
  entry->name = strdup(name);
  entry->alloca_ref = alloca_ref;
  entry->next = var_table;
  var_table = entry;
}

LLVMValueRef var_lookup(const char *name) {
  for (VarEntry *v = var_table; v != NULL; v = v->next) {
    if (strcmp(v->name, name) == 0)
      return v->alloca_ref;
  }
  return NULL; // not found
}

extern LLVMModuleRef module;
extern LLVMBuilderRef builder;

LLVMValueRef get_or_declare_printf() {
  LLVMValueRef printf_func = LLVMGetNamedFunction(module, "printf");
  if (printf_func)
    return printf_func;

  // Parameter 1: i8* (char*)
  LLVMTypeRef param_types[] = {LLVMPointerType(LLVMInt8Type(), 0)};

  // Create function type: int (i32) return, varargs = true (for printf)
  LLVMTypeRef printf_type =
      LLVMFunctionType(LLVMInt32Type(), param_types, 1, 1);

  return LLVMAddFunction(module, "printf", printf_type);
}

void declare_out_functions() {
  LLVMValueRef printf_func = get_or_declare_printf();
  LLVMTypeRef i8ptr = LLVMPointerType(LLVMInt8Type(), 0);

  /* SAVE builder state */
  saved_block = LLVMGetInsertBlock(builder);

  /* outs(string) */
  LLVMTypeRef outs_type = LLVMFunctionType(LLVMInt32Type(), &i8ptr, 1, 0);
  LLVMValueRef outs_func = LLVMAddFunction(module, "outs", outs_type);
  symtab_insert("outs", outs_func, 1);

  {
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(outs_func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMValueRef arg = LLVMGetParam(outs_func, 0);
    LLVMValueRef fmt = LLVMBuildGlobalStringPtr(builder, "%s", "outs_fmt");

    LLVMValueRef args[] = {fmt, arg};
    LLVMTypeRef printf_type = LLVMGlobalGetValueType(printf_func);
    LLVMBuildCall2(builder, printf_type, printf_func, args, 2, "");
    LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));
  }

  /* out(string) */
  LLVMTypeRef out_type = LLVMFunctionType(LLVMInt32Type(), &i8ptr, 1, 0);
  LLVMValueRef out_func = LLVMAddFunction(module, "out", out_type);
  symtab_insert("out", out_func, 1);

  {
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(out_func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMValueRef arg = LLVMGetParam(out_func, 0);
    LLVMValueRef fmt = LLVMBuildGlobalStringPtr(builder, "%s\n", "out_fmt");

    LLVMValueRef args[] = {fmt, arg};
    LLVMTypeRef printf_type = LLVMGlobalGetValueType(printf_func);
    LLVMBuildCall2(builder, printf_type, printf_func, args, 2, "");
    LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));
  }

  /* RESTORE builder state */
  if (saved_block)
    LLVMPositionBuilderAtEnd(builder, saved_block);
}

int collect_args(ASTNode *arg, LLVMValueRef *args) {
  int count = 0;
  while (arg) {
    if (arg->type == NODE_NUM) {
      args[count++] = LLVMConstInt(LLVMInt32Type(), arg->int_value, 0);
    }
    // identifiers can be supported later
    arg = arg->next;
  }
  return count;
}

LLVMValueRef generate_llvm_expr(ASTNode *node) {
  if (!node)
    return NULL;

  switch (node->type) {
  case NODE_NUM: {
    // Integer literal
    return LLVMConstInt(LLVMInt32Type(), node->int_value, 0);
  }
  case NODE_STR: {
    // String literal
    return LLVMBuildGlobalStringPtr(builder, node->value, "str");
  }
  case NODE_CALL: {
    LLVMValueRef args[10];
    int argc = 0;

    for (ASTNode *a = node->left; a; a = a->next) {
      args[argc++] = generate_llvm_expr(a); // recursively generate nested calls
    }

    LLVMValueRef func = symtab_lookup(node->value);
    FunctionEntry *func_entry = symtab_lookup_entry(node->value);

    if (argc != func_entry->arity) {
      fprintf(stderr, "expected %d arguments, got %d\n", func_entry->arity,
              argc);
      exit(1);
    }

    if (!func) {
      fprintf(stderr, "undefined function %s\n", node->value);
      exit(1);
    }

    return LLVMBuildCall2(builder, LLVMGlobalGetValueType(func), func, args,
                          argc, "");
  }
  case NODE_VAR: {
    LLVMValueRef alloca_ref = var_lookup(node->value);
    if (!alloca_ref) {
      fprintf(stderr, "Error: variable %s not defined\n", node->value);
      exit(1);
    }

    // the builder
    return LLVMBuildLoad2(builder,                // the builder
                          LLVMTypeOf(alloca_ref), // type of variable
                          alloca_ref,             // pointer to variable
                          node->value             // instruction name
    );
  }

  default:
    fprintf(stderr, "Error: unsupported expression node type %d\n", node->type);
    exit(1);
  }
}

void generate_llvm(ASTNode *node) {
  if (!node)
    return;

  if (node->type == NODE_CALL) {
    while (node) {
      if (node->type == NODE_CALL) {
        generate_llvm_expr(node); // Executes the call
      }

      node = node->right; // go to the next statement
    }
  } else if (node->type == NODE_VAR && node->left != NULL) {
    // This is an assignment
    LLVMValueRef expr_val = generate_llvm_expr(node->left);
    LLVMValueRef alloca_ref = var_lookup(node->value);
    if (!alloca_ref) {
      // First time: create stack allocation
      alloca_ref = LLVMBuildAlloca(builder, LLVMInt32Type(), node->value);
      var_insert(node->value, alloca_ref);
    }
    LLVMBuildStore(builder, expr_val, alloca_ref);
  }
}

void compile_program(ASTNode *root_node) {
  declare_out_functions(); // define output functions

  // PASS 1: Define all function signatures
  ASTNode *curr = root_node;
  while (curr) {
    if (curr->type == NODE_FUNC) {
      LLVMTypeRef func_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
      LLVMValueRef func = LLVMAddFunction(module, curr->value, func_type);

      // Add to your symbol table/hash map
      symtab_insert(curr->value, func, 0);
    }
    curr = curr->next;
  }

  // PASS 2: Generate function bodies
  curr = root_node;
  while (curr) {
    if (curr->type == NODE_FUNC) {
      LLVMValueRef func = symtab_lookup(curr->value);
      LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
      LLVMPositionBuilderAtEnd(builder, entry);

      // Generate the statements inside the function
      generate_llvm(curr->left);

      // Explicit return from the 'end <value>'
      int rv = curr->right ? curr->right->int_value : 0;
      LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), rv, 0));
    }
    curr = curr->next;
  }
}

int compile_to_object(LLVMModuleRef module, const char *filename) {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();

  char *error = NULL;
  const char *triple = LLVMGetDefaultTargetTriple();

  LLVMTargetRef target;
  if (LLVMGetTargetFromTriple(triple, &target, &error)) {
    fprintf(stderr, "Target Error: %s\n", error);
    return 1;
  }

  LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
      target, triple, "generic", "", LLVMCodeGenLevelDefault, LLVMRelocDefault,
      LLVMCodeModelDefault);

  // Output to a .o file
  if (LLVMTargetMachineEmitToFile(machine, module, (char *)filename,
                                  LLVMObjectFile, &error)) {
    fprintf(stderr, "Emit Error: %s\n", error);
    LLVMDisposeMessage(error);
  }

  LLVMDisposeTargetMachine(machine);
  return 0;
}
