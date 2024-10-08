#
# Copyright (C) 2023, Inria
# GRAPHDECO research group, https://team.inria.fr/graphdeco
# All rights reserved.
#
# This software is free for non-commercial, research and evaluation use 
# under the terms of the LICENSE.md file.
#
# For inquiries contact  george.drettakis@inria.fr
#

from setuptools import setup
from torch.utils.cpp_extension import CppExtension, BuildExtension
import os
os.path.dirname(os.path.abspath(__file__))

# # 获取当前文件路径
current_dir = os.path.dirname(os.path.abspath(__file__))
# build_dir = os.path.join(current_dir, 'build')  # 假设静态库输出到 build/lib 目录

# setup(
#     name="c_diff_gaussian_rasterization",
#     packages=['c_diff_gaussian_rasterization'],
#     ext_modules=[
#         CppExtension(
#             name="diff_gaussian_rasterization._C",
#             sources=[
#             "rasterize_points.cpp",
#             "ext.cpp"
#             ],
#             include_dirs=[os.path.join(current_dir, 'cpu_rasterizer')],
#             library_dirs=[build_dir],
#             libraries=['CpuRasterizer'],  # 静态库名，不用加前缀 lib
#             extra_compile_args={
#                 'cxx': ['-O3'],
#                 'c': ['-std=c11']  # 明确设置 C 编译器选项，避免冲突
#             })
#         ],
#     cmdclass={
#         'build_ext': BuildExtension
#     }
# )

setup(
    name="c_diff_gaussian_rasterization",
    packages=['c_diff_gaussian_rasterization'],
    ext_modules=[
        CppExtension(
            name="c_diff_gaussian_rasterization._C",
            sources=[
                "rasterize_points.cpp",
                "ext.cpp",
                # 将所有需要的 C 源文件添加到这里
                "cpu_rasterizer/forward.c",
                "cpu_rasterizer/rasterizer_impl.c",
                "cpu_rasterizer/auxiliary.c",
                # "cpu_rasterizer/render.c"
            ],
            include_dirs=[os.path.join(current_dir, 'cpu_rasterizer')],
            extra_compile_args={
                'cxx': ['-O3', '-fPIC'],
                'c': ['-O3', '-std=c11', '-fPIC']
            })
        ],
    cmdclass={
        'build_ext': BuildExtension
    }
)