# 1. Paths
SRC_DIR  := src
GEN_DIR  := gen
BIN_DIR  := bin
TARGET   := $(BIN_DIR)/chara

# 2. LLVM Config
LLVM_PATH    := $(shell brew --prefix llvm)
LLVM_CONFIG  := $(LLVM_PATH)/bin/llvm-config
CC           := $(LLVM_PATH)/bin/clang
CFLAGS       := $(shell $(LLVM_CONFIG) --cflags) -I$(GEN_DIR) -I$(SRC_DIR)
LDFLAGS      := $(shell $(LLVM_CONFIG) --ldflags)
LIBS         := $(shell $(LLVM_CONFIG) --libs core analysis native executionengine interpreter mcjit)

# 3. Source & Object Discovery
# Hard-list the names to ensure they are always tracked
SRC_FILES := main.c ast.c codegen.c
GEN_FILES := parser.tab.c lex.yy.c

# Tell Make to look in these folders for any .c or .y/.l files
VPATH = $(SRC_DIR):$(GEN_DIR)

# Flatten objects: all will be bin/filename.o
OBJS := $(addprefix $(BIN_DIR)/, $(SRC_FILES:.c=.o) $(GEN_FILES:.c=.o))

# 4. Build Rules
all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	@$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $(TARGET)

# ONE Rule to rule them all: 
# This looks in VPATH (src/ and gen/) to find the .c file
$(BIN_DIR)/%.o: %.c | $(BIN_DIR)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# 5. Generator Rules
$(GEN_DIR)/parser.tab.c $(GEN_DIR)/parser.tab.h: $(SRC_DIR)/parser.y | $(GEN_DIR)
	@echo "Bison: Generating parser..."
	@bison -d -o $(GEN_DIR)/parser.tab.c $(SRC_DIR)/parser.y

$(GEN_DIR)/lex.yy.c: $(SRC_DIR)/chara.l $(GEN_DIR)/parser.tab.h | $(GEN_DIR)
	@echo "Flex: Generating lexer..."
	@flex -o $(GEN_DIR)/lex.yy.c $(SRC_DIR)/chara.l

# 6. Critical Dependencies
# This forces the generators to run BEFORE any object files are compiled
$(OBJS): $(GEN_DIR)/parser.tab.h


$(BIN_DIR) $(GEN_DIR):
	@mkdir -p $@

clean:
	@rm -rf $(BIN_DIR) $(GEN_DIR)

# 7. Run Rule
# Accepts an optional 'file' argument, defaults to examples/main.chr
# Usage: make run or make run file=test.chr
run: $(TARGET)
	@echo "--- Running Chara ---"
	@./$(TARGET) $(filter-out $@,$(MAKECMDGOALS))

# Ensure examples/main.chr exists for the default run
examples/main.chr:
	@mkdir -p examples
	@echo ':main = do\n  out "Hello, 2026!";\nend' > examples/main.chr
