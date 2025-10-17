.PHONY: all build test clean

all: build

build:
	@mkdir -p build && cd build && \
		cmake .. && cmake --build . --config Release

clean:
	@rm -rf build
