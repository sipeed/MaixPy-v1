SRC_C := $(wildcard *.c)
SRC_CPP := $(wildcard *.cpp)
SRC_C_OBJ := $(SRC_C:.c=.o)
SRC_CXX_OBJ := $(SRC_CPP:.cpp=.o)
SRC_C_OUTPUT_OBJ := $(addprefix $(OUTPUT_DIR)/, $(SRC_C_OBJ))
SRC_CXX_OUTPUT_OBJ := $(addprefix $(OUTPUT_DIR)/, $(SRC_CXX_OBJ))

$(SRC_C_OUTPUT_OBJ): $(SRC_C_OBJ) 
$(SRC_CXX_OUTPUT_OBJ): $(SRC_CXX_OBJ)

$(SRC_CXX_OBJ):%.o:%.cpp
	@mkdir -p $(OUTPUT_DIR)
	@$(CXX)  -o $(OUTPUT_DIR)/$@ -c $< $(INCLUDE) $(CXXFLAGS) -lstdc++
$(SRC_C_OBJ):%.o:%.c
	@mkdir -p $(OUTPUT_DIR)
	@echo -e ${GREEN}"CC "$< ${NC}
	@$(CC)  -o $(OUTPUT_DIR)/$@ -c $< $(INCLUDE) $(CFLAGS) -lstdc++
