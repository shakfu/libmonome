# libmonome (cython-centric)

A variant specialized for building `monome` cython extension module using modern methods.


## Note on this WIP fork

Changes:

- Specialized to compile cython monome module

- Convert to `uv` + `scikit-build-core`

- Drop waf

- Drop `binding` folder

- Drop shared library building in favor of static builds

Building

```sh
uv sync
```

to setup

```sh
uv build
```

to build a wheel

## Original README

libmonome is a library for easy interaction with monome devices. It
currently runs on Linux (on which it is primarily developed), OpenBSD, and
Darwin/OS X.

It was developed to make monome devices easy to use with programming
languages like C and Python, and adding wrappers for use in your favorite
language with a suitable FFI is straightforward.

libmonome has support for 40h and series devices through a unified API,
and by default includes a third backend which wraps around the OSC
protocol and presents the same API as a physical device. This means that a
program written using libmonome can, at runtime, decide whether to
communicate with a running monomeserial instance over OSC or whether to
access the physical device directly.

build like:

```sh
$ mkdir build; cd build
$ cmake ..
$ make; sudo make install
```

libmonome was written by william light (visinin on the monome forums).
You can contact him at <wrl@illest.net>.


