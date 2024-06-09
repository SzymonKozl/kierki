CXX = g++
CXXFLAGS = -std=c++20 -Icommon -Iserver -Iclient
LDFLAGS = -lboost_program_options

# Directories
CLIENT_DIR = client
SERVER_DIR = server
COMMON_DIR = common

# Source files
CLIENT_SRCS = $(CLIENT_DIR)/client.cpp $(CLIENT_DIR)/player_auto.cpp $(CLIENT_DIR)/player_console.cpp $(CLIENT_DIR)/player.cpp $(CLIENT_DIR)/strategy.cpp
SERVER_SRCS = $(SERVER_DIR)/game_rules.cpp $(SERVER_DIR)/io_worker_connect.cpp $(SERVER_DIR)/io_worker.cpp $(SERVER_DIR)/io_worker_handler.cpp $(SERVER_DIR)/io_worker_mgr.cpp $(SERVER_DIR)/job_queue.cpp $(SERVER_DIR)/server.cpp
COMMON_SRCS = $(COMMON_DIR)/card.cpp $(COMMON_DIR)/logger.cpp $(COMMON_DIR)/message.cpp $(COMMON_DIR)/network_msg_parser.cpp $(COMMON_DIR)/send_job.cpp $(COMMON_DIR)/utils.cpp

# Object files
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o) $(COMMON_SRCS:.cpp=.o)
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o) $(COMMON_SRCS:.cpp=.o)

# Targets
TARGETS = kierki-server kierki-client

# Phony targets
.PHONY: all clean

# Default target
all: $(TARGETS)

# Executables
kierki-server: kierki-server.o $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

kierki-client: kierki-client.o $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(CLIENT_OBJS) $(SERVER_OBJS) kierki-server.o kierki-client.o $(TARGETS) sk448304.tgz sk448304.tar

pack:
	tar -cf sk448304.tar ./common ./server ./client kierki-server.cpp kierki-client.cpp Makefile
	gzip sk448304.tar -S .tgz -c > sk448304.tgz
	rm sk448304.tar
