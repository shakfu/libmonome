import os
import platform
from setuptools import setup, Extension
from Cython.Build import cythonize

PLATFORM = platform.system()

os.system("cython -o build/monome.c bindings/python/monome.pyx")


setup(
    name='monome',
    version="0.1.0",
    ext_modules=cythonize([
        Extension(
            name="monome",
            sources=["build/monome.c"],
            # sources=["bindings/python/monome.pyx"],
            define_macros=[],
            include_dirs=[
                "public",
            ],
            libraries = ["udev"] if PLATFORM=="Linux" else [],
            library_dirs=[],
            extra_objects=[
                "build/libmonome.a"
            ],
            extra_compile_args=[],
            extra_link_args=[],
        )
    ],
    build_dir="build/cython",
))
