CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -O2
INCLUDES = -Ids -Isrc -DHAVE_SIMS

DS_SOURCES = ds/Graph.cpp ds/Queue.cpp ds/Stack.cpp
APP_SOURCES = src/Parser.cpp src/SimParser.cpp src/TimingAnalyzer.cpp src/VCDWriter.cpp \
              src/EventSim.cpp src/LevelizedSim.cpp

.PHONY: all test check clean generate-adders
all: sim test_ds test_analysis test_circuits

sim: src/main.cpp $(APP_SOURCES) $(DS_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

test_ds: tests/test_ds.cpp $(DS_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

test_analysis: tests/test_analysis.cpp $(APP_SOURCES) $(DS_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

test_circuits: tests/test_circuits.cpp $(APP_SOURCES) $(DS_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

test: test_ds test_analysis test_circuits
	./test_ds
	./test_analysis tests
	./test_circuits

check: test

generate-adders:
	python3 tools/gen_adders.py tests

clean:
	rm -f sim test_ds test_analysis test_circuits *.vcd
