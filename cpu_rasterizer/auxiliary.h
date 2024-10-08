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

#ifndef CPU_RASTERIZER_AUXILIARY_H_INCLUDED
#define CPU_RASTERIZER_AUXILIARY_H_INCLUDED

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#define BLOCK_SIZE (BLOCK_X * BLOCK_Y)
#define NUM_WARPS (BLOCK_SIZE/32)


// 定义数据结构
typedef struct {
    float x, y;
} float2;

typedef struct {
    float x, y, z;
} float3;

typedef struct {
    float x, y, z, w;
} float4;

typedef struct {
    unsigned int x, y;
} uint2;

typedef struct {
    unsigned int x, y, z;
} dim3;

static inline float cpu_rasterizer_ndc2Pix(float v, int S)
{
	return ((v + 1.0) * S - 1.0) * 0.5;
}

static inline void cpu_rasterizer_getRect(const float2 p, int max_radius, uint2* rect_min, uint2* rect_max, dim3 grid) {
    rect_min->x = (unsigned int)fmin((float)grid.x, fmax(0.0f, floor((p.x - max_radius) / BLOCK_X)));
    rect_min->y = (unsigned int)fmin((float)grid.y, fmax(0.0f, floor((p.y - max_radius) / BLOCK_Y)));
    rect_max->x = (unsigned int)fmin((float)grid.x, fmax(0.0f, ceil((p.x + max_radius + BLOCK_X - 1) / BLOCK_X)));
    rect_max->y = (unsigned int)fmin((float)grid.y, fmax(0.0f, ceil((p.y + max_radius + BLOCK_Y - 1) / BLOCK_Y)));
}

static inline float3 cpu_rasterizer_transformPoint4x3(const float3 p, const float* matrix) {
    float3 transformed = {
        matrix[0] * p.x + matrix[4] * p.y + matrix[8]  * p.z + matrix[12],
        matrix[1] * p.x + matrix[5] * p.y + matrix[9]  * p.z + matrix[13],
        matrix[2] * p.x + matrix[6] * p.y + matrix[10] * p.z + matrix[14],
    };
    return transformed;
}

static inline float4 cpu_rasterizer_transformPoint4x4(const float3 p, const float* matrix) {
    float4 transformed = {
        matrix[0] * p.x + matrix[4] * p.y + matrix[8]  * p.z + matrix[12],
        matrix[1] * p.x + matrix[5] * p.y + matrix[9]  * p.z + matrix[13],
        matrix[2] * p.x + matrix[6] * p.y + matrix[10] * p.z + matrix[14],
        matrix[3] * p.x + matrix[7] * p.y + matrix[11] * p.z + matrix[15]
    };
    return transformed;
}

// // 矩阵转换函数，将向量乘以4x3矩阵
// float3 transformVec4x3(const float3 p, const float* matrix)
// {
//     float3 transformed = {
//         matrix[0] * p.x + matrix[4] * p.y + matrix[8]  * p.z,
//         matrix[1] * p.x + matrix[5] * p.y + matrix[9]  * p.z,
//         matrix[2] * p.x + matrix[6] * p.y + matrix[10] * p.z,
//     };
//     return transformed;
// }

// // 矩阵转换函数，将向量乘以4x3矩阵的转置
// float3 transformVec4x3Transpose(const float3 p, const float* matrix)
// {
//     float3 transformed = {
//         matrix[0] * p.x + matrix[1] * p.y + matrix[2]  * p.z,
//         matrix[4] * p.x + matrix[5] * p.y + matrix[6]  * p.z,
//         matrix[8] * p.x + matrix[9] * p.y + matrix[10] * p.z,
//     };
//     return transformed;
// }

// // 计算向量归一化的z方向导数
// float dnormvdz(const float3 v, const float3 dv)
// {
//     float sum2 = v.x * v.x + v.y * v.y + v.z * v.z;
//     float invsum32 = 1.0f / sqrt(sum2 * sum2 * sum2);
//     float result = (-v.x * v.z * dv.x - v.y * v.z * dv.y + (sum2 - v.z * v.z) * dv.z) * invsum32;
//     return result;
// }

// // 计算向量归一化对向量v的导数
// float3 dnormvdv(const float3 v, const float3 dv)
// {
//     float sum2 = v.x * v.x + v.y * v.y + v.z * v.z;
//     float invsum32 = 1.0f / sqrt(sum2 * sum2 * sum2);

//     float3 result;
//     result.x = ((sum2 - v.x * v.x) * dv.x - v.x * (v.y * dv.y + v.z * dv.z)) * invsum32;
//     result.y = ((sum2 - v.y * v.y) * dv.y - v.y * (v.x * dv.x + v.z * dv.z)) * invsum32;
//     result.z = ((sum2 - v.z * v.z) * dv.z - v.z * (v.x * dv.x + v.y * dv.y)) * invsum32;
//     return result;
// }

// // 计算四维向量归一化对向量v的导数
// float4 dnormvdv4(const float4 v, const float4 dv)
// {
//     float sum2 = v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
//     float invsum32 = 1.0f / sqrt(sum2 * sum2 * sum2);

//     float vdv_sum = v.x * dv.x + v.y * dv.y + v.z * dv.z + v.w * dv.w;
//     float4 result;
//     result.x = ((sum2 - v.x * v.x) * dv.x - v.x * (vdv_sum - v.x * dv.x)) * invsum32;
//     result.y = ((sum2 - v.y * v.y) * dv.y - v.y * (vdv_sum - v.y * dv.y)) * invsum32;
//     result.z = ((sum2 - v.z * v.z) * dv.z - v.z * (vdv_sum - v.z * dv.z)) * invsum32;
//     result.w = ((sum2 - v.w * v.w) * dv.w - v.w * (vdv_sum - v.w * dv.w)) * invsum32;
//     return result;
// }

// // Sigmoid函数
// float sigmoid(float x)
// {
//     return 1.0f / (1.0f + exp(-x));
// }

// 判断点是否在视锥内
int cpu_rasterizer_in_frustum(int idx,
    const float* orig_points,
    const float* viewmatrix,
    const float* projmatrix,
    int prefiltered,
    float3* p_view);

#endif