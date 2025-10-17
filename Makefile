.PHONY: all build python test clean

all: build

build:
	@mkdir -p build && cd build && \
		cmake .. && cmake --build . --config Release

python:
	@mkdir -p build && cd build && \
		cmake .. -DBUILD_PYTHON_EXTENSION=ON && \
		cmake --build . --config Release

clean:
	@rm -rf build
