SRC=main.cpp
OBJ=$(SRC:.cpp=.o)
TARGET=tlaz

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -g -o $(TARGET) $(CXX_FLAGS) $(LD_FLAGS) $<

%.o : %.cpp
	$(CXX) -g -c -o $@ $(CXX_FLAGS) $<

clean:
	rm -rf $(OBJ)
	rm -rf $(TARGET)
