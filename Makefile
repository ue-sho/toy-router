CC = g++
CFLAGS = -Wall -Wextra -std=c++17 -pthread
SRC_DIR = src
SRCS = $(SRC_DIR)/main.cpp $(SRC_DIR)/router.cpp $(SRC_DIR)/netutil.cpp $(SRC_DIR)/ip2mac.cpp $(SRC_DIR)/send_buf.cpp $(SRC_DIR)/base.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = router

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean