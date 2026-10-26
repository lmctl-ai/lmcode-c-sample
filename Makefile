CC ?= cc
CXX ?= c++
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Werror -pedantic
BUILD ?= build
SDK ?= .
INCLUDES=-Isrc -Iinclude -Istdlib -I"$(BUILD)"
.PHONY: all test clean
all: $(BUILD)/lmcode-c-sample
$(BUILD)/sample_schema.hpp: sample-suite/sample.dsl.json scripts/embed-schema.py
	mkdir -p $(BUILD)
	python3 scripts/embed-schema.py $< $@
$(BUILD)/shop.o: src/shop.c src/shop.h include/lmcode_c.h
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
$(BUILD)/lmcode_c.o: src/lmcode_c.cpp include/lmcode_c.h include/dslvm.hpp $(wildcard stdlib/*.hpp)
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
$(BUILD)/main.o: src/main.cpp src/shop.h include/lmcode_c.h include/dslvm.hpp $(BUILD)/sample_schema.hpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
$(BUILD)/lmcode-c-sample: $(BUILD)/main.o $(BUILD)/shop.o $(BUILD)/lmcode_c.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@
test: all
	bash sample-suite/run-suite.sh "$(abspath $(BUILD)/lmcode-c-sample)"
clean:
	rm -rf $(BUILD)
