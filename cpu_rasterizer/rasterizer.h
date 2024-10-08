/*
 * Copyright (C) 2023, Inria
 * GRAPHDECO research group, https://team.inria.fr/graphdeco
 * All rights reserved.
 *
 * This software is free for non-commercial, research and evaluation use
 * under the terms of the LICENSE.md file.
 *
 * For inquiries contact  george.drettakis@inria.fr
 */

#ifndef CPU_RASTERIZER_H_INCLUDED
#define CPU_RASTERIZER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

void cpu_rasterizer_markVisible(
	int P,
	float *means3D,
	float *viewmatrix,
	float *projmatrix,
	int *present);

// 定义一个函数类型，它接受一个 size_t 类型的参数并返回 char* 类型
typedef char* (*BufferAllocator)(size_t);

int cpu_rasterizer_forward(
	// BufferAllocator geometryBuffer,
	// BufferAllocator binningBuffer,
	// BufferAllocator imageBuffer,
	const int P, int D, int M,
	const float *background,
	const int width, int height,
	const float *means3D,
	const float *shs,
	const float *colors_precomp,
	const float *opacities,
	const float *scales,
	const float scale_modifier,
	const float *rotations,
	const float *cov3D_precomp,
	const float *viewmatrix,
	const float *projmatrix,
	const float *cam_pos,
	const float tan_fovx, float tan_fovy,
	const int prefiltered,
	float *out_color,
	int *radii,
	int debug);

#ifdef __cplusplus
}
#endif

#endif