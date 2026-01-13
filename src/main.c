#include "ast.h"
#include "parser.tab.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void compile_program(ASTNode *root_node);
int compile_to_object(LLVMModuleRef module, const char *filename);

char *remove_extension(const char *filename);

LLVMModuleRef module;
LLVMBuilderRef builder;

extern FILE *yyin;
extern int yyparse();
extern struct ASTNode *root; // From parser.y

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename.chr>\n", argv[0]);
    return 1;
  }

  // Open the file
  FILE *file = fopen(argv[1], "r");
  if (!file) {
    perror("Error opening file");
    return 1;
  }

  // Set Flex to read from the file instead of stdin
  yyin = file;

  // extern int yydebug;
  // yydebug = 1; // This prints every rule Bison tries to match
  if (yyparse() == 0) {
    // Initialize LLVM
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    module = LLVMModuleCreateWithName("chara_module");
    builder = LLVMCreateBuilder();

    // Generate IR from the AST
    compile_program(root);

    // Dump the IR to terminal
    // char *ir = LLVMPrintModuleToString(module);
    // "\n--- Generated LLVM IR ---\n%s\n", ir);

    char *base_path = remove_extension(argv[1]);

    // Create filenames like "examples/main.o" and "examples/main"
    char obj_name[256];
    char exe_name[256];
    snprintf(obj_name, sizeof(obj_name), "%s.o", base_path);
    snprintf(exe_name, sizeof(exe_name), "%s", base_path);

    if (argc > 2) {
      if (strcmp(argv[2], "-llvm") == 0) {
        LLVMDumpModule(module);
      }
    }

    int status = compile_to_object(module, obj_name);
    if (status != 0) {
      fprintf(stderr, "could not compile program to object file\n");
      return status;
    }

    // link using clean base name
    char link_cmd[512];
    snprintf(link_cmd, sizeof(link_cmd), "clang %s -o %s", obj_name, exe_name);

    status = system(link_cmd);
    if (status != 0) {
      fprintf(stderr, "could not compile object file\n");
      return status;
    }

    printf("Successfully compiled: %s\n", exe_name);

    free(base_path);

    // Clean up
    // LLVMDisposeMessage(ir);
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
  }

  fclose(file);
  return 0;
}

char *remove_extension(const char *filename) {
  char *new_name = strdup(filename);
  if (!new_name)
    return NULL;

  // Find the last occurrence of '.'
  char *last_dot = strrchr(new_name, '.');

  // Ensure there is a dot and it's not part of a hidden file path (e.g., ./)
  if (last_dot != NULL) {
    *last_dot = '\0';
  }

  return new_name;
}
