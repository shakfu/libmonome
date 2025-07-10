LIBMONOME := build/libmonome.a


.phony: all build python clean


all: build


$(LIBMONOME):
	@mkdir -p build && cd build && \
		cmake .. && \
		cmake --build . --config Release


build: $(LIBMONOME)


python: build
	@uv build


clean:
	@rm -rf build dist

