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
	mkdir -p $(BUILD); python3 scripts/embed-schema.py $< $@
$(BUILD)/lmcode-c-sample: src/main.cpp src/shop.c src/shop.h src/lmcode_c.cpp $(BUILD)/sample_schema.hpp include/lmcode_c.h include/dslvm.hpp
	mkdir -p $(BUILD); $(CC) $(CFLAGS) $(INCLUDES) -c src/shop.c -o $(BUILD)/shop.o; $(CXX) $(CXXFLAGS) $(INCLUDES) -c src/lmcode_c.cpp -o $(BUILD)/lmcode_c.o; $(CXX) $(CXXFLAGS) $(INCLUDES) src/main.cpp $(BUILD)/shop.o $(BUILD)/lmcode_c.o -o $@
test: all
	bash sample-suite/run-suite.sh "$(abspath $(BUILD)/lmcode-c-sample)"
clean:
	rm -rf $(BUILD)
