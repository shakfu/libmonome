LIBMONOME := build/libmonome.a
PYEXT := monome.*.so

.phony: all build libmonome python-ext clean reset

all: build

$(LIBMONOME):
	@mkdir -p build && cd build && \
		cmake .. && \
		cmake --build . --config Release

$(PYEXT):
	@python3 setup.py build_ext --inplace
	@rm monome.c
	@mv src/monome.*.so .	


build: $(LIBMONOME) $(PYEXT)


test: clean build
	@python3 examples/python/simple.py


clean:
	@rm -rf build dist monome.*.so


reset: clean
	@rm -rf .venv
