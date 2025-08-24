# Compiler and flags
CXX = g++
# -std=c++17 is recommended for modern C++ features.
# -Wall and -Wextra enable all important warnings.
# -pthread is CRUCIAL for linking the std::thread library.
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -Isource/network -Isource/network/observer

# The name of your final executable
TARGET = test_nic

# All your .cpp source files. For now, it's just the main test file.
# If you add .cpp files for other classes, add them here.
SOURCES = test_nic.cpp

# Default rule: running "make" will build the target
all: $(TARGET)

# Rule to link the final executable
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to clean up compiled files
clean:
	rm -f $(TARGET)

# Phony targets are not actual files
.PHONY: all clean