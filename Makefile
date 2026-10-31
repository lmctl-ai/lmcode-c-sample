VERSION ?= 0.4.0
CC ?= cc
CXX ?= c++
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Werror -pedantic
BUILD ?= build
SDK ?= .
INCLUDES=-Isrc -Iinclude -Istdlib -I"$(BUILD)"
INCLUDES += '-DSAMPLE_VERSION="$(VERSION)"'
.PHONY: all test test-domain clean
all: $(BUILD)/lmcode-c-sample
$(BUILD)/domain_test: tests/domain_test.c src/domain.c src/domain.h
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -Isrc tests/domain_test.c src/domain.c -lm -o $@
test-domain: $(BUILD)/domain_test
	$(BUILD)/domain_test
$(BUILD)/sample_schema.hpp: sample-suite/sample.dsl.json scripts/embed-schema.py
	mkdir -p $(BUILD)
	python3 scripts/embed-schema.py $< $@
$(BUILD)/domain.o: src/domain.c src/domain.h
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
$(BUILD)/bindings.o: src/bindings.cpp src/bindings.hpp src/domain.h include/dslvm.hpp $(wildcard stdlib/*.hpp) $(BUILD)/sample_schema.hpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
$(BUILD)/main.o: src/main.cpp src/bindings.hpp src/domain.h include/dslvm.hpp $(BUILD)/sample_schema.hpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
$(BUILD)/lmcode-c-sample: $(BUILD)/main.o $(BUILD)/domain.o $(BUILD)/bindings.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@
test: all
	$(MAKE) test-domain
	python3 sample-suite/check-invocations.py "$(abspath $(BUILD)/lmcode-c-sample)"
	bash sample-suite/run-suite.sh "$(abspath $(BUILD)/lmcode-c-sample)"
clean:
	rm -rf $(BUILD)

ifeq ($(OS),Windows_NT)
CXXFLAGS += -municode
endif
