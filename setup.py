from setuptools import setup, Extension
import pybind11

module = Extension(
    "WinFile",
    sources=["copier.cpp"],  # C++ 源文件
    include_dirs=[
        pybind11.get_include(),
        "E:\\vcpkg\\installed\\x64-windows\\include",
        "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.22000.0\\winrt",
    ],  # 包含 pybind11 头文件
    libraries=["shell32", "ole32", "oleaut32", "uuid", "shcore", "fmt"],  # 链接的库
    language="c++",
    extra_compile_args=["/std:c++17", "/O2", "/utf-8"],
)

setup(
    name="WinFile",
    version="1.0",
    description="A Python module for copying files using Explorer API",
    ext_modules=[module],
)
