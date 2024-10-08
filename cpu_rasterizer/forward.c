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

#include "forward.h"
#include "auxiliary.h"
#include<math.h>
static inline int min_int(int a, int b) {
    return (a < b) ? a : b;
}
// Vector subtraction: result = a - b
static inline float3 vec3_sub(float3 a, float3 b)
{
    float3 result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return result;
}

// Vector division by scalar: result = a / scalar
static inline float3 vec3_div_scalar(float3 a, float scalar)
{
    float3 result = {a.x / scalar, a.y / scalar, a.z / scalar};
    return result;
}

// Vector multiplication by scalar: result = a * scalar
static inline float3 vec3_mul_scalar(float3 a, float scalar)
{
    float3 result = {a.x * scalar, a.y * scalar, a.z * scalar};
    return result;
}

// Vector addition: result = a + b
static inline float3 vec3_add(float3 a, float3 b)
{
    float3 result = {a.x + b.x, a.y + b.y, a.z + b.z};
    return result;
}

// Vector dot product: result = a . b
static inline float vec3_dot(float3 a, float3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Vector length: result = ||a||
static inline float vec3_length(float3 a)
{
    return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

// Component-wise maximum of vector and scalar
static inline float3 vec3_max_scalar(float3 a, float scalar)
{
    float3 result = {fmax(a.x, scalar), fmax(a.y, scalar), fmax(a.z, scalar)};
    return result;
}

// Component-wise less-than comparison with scalar
static inline void vec3_less_than_scalar(float3 a, float scalar, int *clamped)
{
    clamped[0] = (a.x < scalar) ? 1 : 0;
    clamped[1] = (a.y < scalar) ? 1 : 0;
    clamped[2] = (a.z < scalar) ? 1 : 0;
}

float3 computeColorFromSH(int idx, int deg, int max_coeffs, const float3 *means, float3 campos, const float *shs, int *clamped)
{
    // The implementation is loosely based on code for
    // "Differentiable Point-Based Radiance Fields for
    // Efficient View Synthesis" by Zhang et al. (2022)
    float3 pos = means[idx];
    float3 dir = vec3_sub(pos, campos);
    float length = vec3_length(dir);
    dir = vec3_div_scalar(dir, length);

    const float3 *sh = (const float3 *)shs + idx * max_coeffs;
    float3 result = vec3_mul_scalar(sh[0], SH_C0);

    if (deg > 0)
    {
        float x = dir.x;
        float y = dir.y;
        float z = dir.z;

        result = vec3_sub(result,
                          vec3_mul_scalar(sh[1], SH_C1 * y));
        result = vec3_add(result,
                          vec3_mul_scalar(sh[2], SH_C1 * z));
        result = vec3_sub(result,
                          vec3_mul_scalar(sh[3], SH_C1 * x));

        if (deg > 1)
        {
            float xx = x * x, yy = y * y, zz = z * z;
            float xy = x * y, yz = y * z, xz = x * z;

            result = vec3_add(result,
                              vec3_mul_scalar(sh[4], SH_C2[0] * xy));
            result = vec3_add(result,
                              vec3_mul_scalar(sh[5], SH_C2[1] * yz));
            result = vec3_add(result,
                              vec3_mul_scalar(sh[6], SH_C2[2] * (2.0f * zz - xx - yy)));
            result = vec3_add(result,
                              vec3_mul_scalar(sh[7], SH_C2[3] * xz));
            result = vec3_add(result,
                              vec3_mul_scalar(sh[8], SH_C2[4] * (xx - yy)));

            if (deg > 2)
            {
                result = vec3_add(result,
                                  vec3_mul_scalar(sh[9], SH_C3[0] * y * (3.0f * xx - yy)));
                result = vec3_add(result,
                                  vec3_mul_scalar(sh[10], SH_C3[1] * xy * z));
                result = vec3_add(result,
                                  vec3_mul_scalar(sh[11], SH_C3[2] * y * (4.0f * zz - xx - yy)));
                result = vec3_add(result,
                                  vec3_mul_scalar(sh[12], SH_C3[3] * z * (2.0f * zz - 3.0f * xx - 3.0f * yy)));
                result = vec3_add(result,
                                  vec3_mul_scalar(sh[13], SH_C3[4] * x * (4.0f * zz - xx - yy)));
                result = vec3_add(result,
                                  vec3_mul_scalar(sh[14], SH_C3[5] * z * (xx - yy)));
                result = vec3_add(result,
                                  vec3_mul_scalar(sh[15], SH_C3[6] * x * (xx - 3.0f * yy)));
            }
        }
    }

    // Add 0.5f to the result
    result = vec3_add(result, (float3){0.5f, 0.5f, 0.5f});

    // Clamp RGB colors to positive values. Keep track of clamped components.
    clamped[3 * idx + 0] = (result.x < 0.0f) ? 1 : 0;
    clamped[3 * idx + 1] = (result.y < 0.0f) ? 1 : 0;
    clamped[3 * idx + 2] = (result.z < 0.0f) ? 1 : 0;

    // Clamp the result to zero
    result = vec3_max_scalar(result, 0.0f);

    return result;
}

// 计算2D协方差矩阵
static inline float3 computeCov2D(const float3 mean, float focal_x, float focal_y, float tan_fovx, float tan_fovy, const float *cov3D, const float *viewmatrix)
{
    // 转换为视图坐标系
    float3 t = cpu_rasterizer_transformPoint4x3(mean, viewmatrix);

    // 限制在视锥内
    float limx = 1.3f * tan_fovx;
    float limy = 1.3f * tan_fovy;
    float txtz = t.x / t.z;
    float tytz = t.y / t.z;
    t.x = fmin(limx, fmax(-limx, txtz)) * t.z;
    t.y = fmin(limy, fmax(-limy, tytz)) * t.z;

    // 计算Jacobian矩阵
    float J[9] = {
        focal_x / t.z, 0.0f, -(focal_x * t.x) / (t.z * t.z),
        0.0f, focal_y / t.z, -(focal_y * t.y) / (t.z * t.z),
        0.0f, 0.0f, 0.0f};

    // 计算W矩阵
    float W[9] = {
        viewmatrix[0], viewmatrix[4], viewmatrix[8],
        viewmatrix[1], viewmatrix[5], viewmatrix[9],
        viewmatrix[2], viewmatrix[6], viewmatrix[10]};

    // 计算T矩阵 T = W * J
    float T[9];
    // 进行矩阵乘法 W * J
    for (int i = 0; i < 3; i++)
    { // 行
        for (int j = 0; j < 3; j++)
        { // 列
            T[i * 3 + j] = 0.0f;
            for (int k = 0; k < 3; k++)
            {
                T[i * 3 + j] += W[i * 3 + k] * J[k * 3 + j];
            }
        }
    }

    // 计算Vrk矩阵
    float Vrk[9] = {
        cov3D[0], cov3D[1], cov3D[2],
        cov3D[1], cov3D[3], cov3D[4],
        cov3D[2], cov3D[4], cov3D[5]};

    // 计算cov矩阵 cov = T^T * Vrk * T
    float TT[9]; // T的转置
    for (int i = 0; i < 3; i++)
    { // 行
        for (int j = 0; j < 3; j++)
        {
            TT[i * 3 + j] = T[j * 3 + i];
        }
    }

    // 对Vrk进行转置
    float Vrk_T[9];
    for (int i = 0; i < 3; i++)
    { // 行
        for (int j = 0; j < 3; j++)
        {
            Vrk_T[i * 3 + j] = Vrk[j * 3 + i];
        }
    }

    float temp[9]; // TT * Vrk_T
    for (int i = 0; i < 3; i++)
    { // 行
        for (int j = 0; j < 3; j++)
        {
            temp[i * 3 + j] = 0.0f;
            for (int k = 0; k < 3; k++)
            {
                temp[i * 3 + j] += TT[i * 3 + k] * Vrk_T[k * 3 + j];
            }
        }
    }

    float cov[9]; // temp * T
    for (int i = 0; i < 3; i++)
    { // 行
        for (int j = 0; j < 3; j++)
        {
            cov[i * 3 + j] = 0.0f;
            for (int k = 0; k < 3; k++)
            {
                cov[i * 3 + j] += temp[i * 3 + k] * T[k * 3 + j];
            }
        }
    }

    // 应用低通滤波器
    cov[0] += 0.3f;
    cov[4] += 0.3f;

    // 提取需要的元素
    float3 result = {cov[0], cov[1], cov[4]};
    return result; // cov_xx, cov_xy, cov_yy
}

// 计算3D协方差矩阵
static inline void computeCov3D(const float3 scale, float mod, const float4 rot, float *cov3D)
{
    // 创建缩放矩阵 S
    float S[9] = {
        mod * scale.x, 0.0f, 0.0f,
        0.0f, mod * scale.y, 0.0f,
        0.0f, 0.0f, mod * scale.z};

    float qx = rot.x;
    float qy = rot.y;
    float qz = rot.z;
    float qw = rot.w;

    // 计算旋转矩阵 R
    float R[9] = {
        1.f - 2.f * (qy * qy + qz * qz), 2.f * (qx * qy - qw * qz), 2.f * (qx * qz + qw * qy),
        2.f * (qx * qy + qw * qz), 1.f - 2.f * (qx * qx + qz * qz), 2.f * (qy * qz - qw * qx),
        2.f * (qx * qz - qw * qy), 2.f * (qy * qz + qw * qx), 1.f - 2.f * (qx * qx + qy * qy)};

    // 计算 M = S * R
    float M[9];
    for (int i = 0; i < 3; i++)
    { // 行
        for (int j = 0; j < 3; j++)
        { // 列
            M[i * 3 + j] = 0.0f;
            for (int k = 0; k < 3; k++)
            {
                M[i * 3 + j] += S[i * 3 + k] * R[k * 3 + j];
            }
        }
    }

    // 计算 Sigma = M^T * M
    float MT[9]; // M的转置
    for (int i = 0; i < 3; i++)
    { // 行
        for (int j = 0; j < 3; j++)
        {
            MT[i * 3 + j] = M[j * 3 + i];
        }
    }

    float Sigma[9];
    for (int i = 0; i < 3; i++)
    { // 行
        for (int j = 0; j < 3; j++)
        { // 列
            Sigma[i * 3 + j] = 0.0f;
            for (int k = 0; k < 3; k++)
            {
                Sigma[i * 3 + j] += MT[i * 3 + k] * M[k * 3 + j];
            }
        }
    }

    // 提取协方差矩阵的上三角部分
    cov3D[0] = Sigma[0]; // cov_xx
    cov3D[1] = Sigma[1]; // cov_xy
    cov3D[2] = Sigma[2]; // cov_xz
    cov3D[3] = Sigma[4]; // cov_yy
    cov3D[4] = Sigma[5]; // cov_yz
    cov3D[5] = Sigma[8]; // cov_zz
}

// 预处理函数
void preprocessCPU(int P, int D, int M,
                   const float *orig_points,
                   const float3 *scales, // 假设是连续存储的float3数组
                   const float scale_modifier,
                   const float4 *rotations, // 假设是连续存储的float4数组
                   const float *opacities,
                   const float *shs,
                   int *clamped,
                   const float *cov3D_precomp,
                   const float *colors_precomp,
                   const float *viewmatrix,
                   const float *projmatrix,
                   const float3 *cam_pos, // 假设是float3
                   const int W, int H,
                   const float tan_fovx, float tan_fovy,
                   const float focal_x, float focal_y,
                   int *radii,
                   float2 *points_xy_image,
                   float *depths,
                   float *cov3Ds,
                   float *rgb,
                   float4 *conic_opacity,
                   const dim3 grid,
                   uint32_t *tiles_touched,
                   int prefiltered)
{
    for (int idx = 0; idx < P; idx++)
    {
        // 初始化
        radii[idx] = 0;
        tiles_touched[idx] = 0;

        float3 p_view;
        if (!cpu_rasterizer_in_frustum(idx, orig_points, viewmatrix, projmatrix, prefiltered, &p_view))
            continue;

        // 投影变换
        float3 p_orig = {orig_points[3 * idx], orig_points[3 * idx + 1], orig_points[3 * idx + 2]};
        float4 p_hom = cpu_rasterizer_transformPoint4x4(p_orig, projmatrix);
        float p_w = 1.0f / (p_hom.w + 0.0000001f);
        float3 p_proj = {p_hom.x * p_w, p_hom.y * p_w, p_hom.z * p_w};

        // 计算3D协方差矩阵
        const float *cov3D;
        if (cov3D_precomp != NULL)
        {
            cov3D = cov3D_precomp + idx * 6;
        }
        else
        {
            computeCov3D(scales[idx], scale_modifier, rotations[idx], cov3Ds + idx * 6);
            cov3D = cov3Ds + idx * 6;
        }

        // 计算2D协方差矩阵
        float3 cov = computeCov2D(p_orig, focal_x, focal_y, tan_fovx, tan_fovy, cov3D, viewmatrix);

        // 计算协方差矩阵的逆
	    float det = (cov.x * cov.z - cov.y * cov.y);
        if (det == 0.0f)
            continue;
        float det_inv = 1.0f / det;
    	float3 conic = { cov.z * det_inv, -cov.y * det_inv, cov.x * det_inv };

        // 计算屏幕空间的范围
        float mid = 0.5f * (cov.x + cov.z);
        float delta = mid * mid - det;
        delta = fmax(delta, 0.1f); // 防止负值
        float sqrt_delta = sqrt(delta);
        float lambda1 = mid + sqrt_delta;
        float lambda2 = mid - sqrt_delta;
        float my_radius = ceil(3.0f * sqrt(fmax(lambda1, lambda2)));
        float2 point_image = {cpu_rasterizer_ndc2Pix(p_proj.x, W), cpu_rasterizer_ndc2Pix(p_proj.y, H)};
        uint2 rect_min, rect_max;
        cpu_rasterizer_getRect(point_image, (int)my_radius, &rect_min, &rect_max, grid);
        if ((rect_max.x - rect_min.x) * (rect_max.y - rect_min.y) == 0)
            continue;

        // 计算颜色
        if (colors_precomp == NULL)
        {
            float color[3];
            float3 campos = *cam_pos;
            float3 result = computeColorFromSH(idx, D, M, (float3*)orig_points, campos, shs, clamped);
            rgb[idx * NUM_CHANNELS + 0] = result.x;
            rgb[idx * NUM_CHANNELS + 1] = result.y;
            rgb[idx * NUM_CHANNELS + 2] = result.z;
        }

        // 存储数据
        depths[idx] = p_view.z;
        radii[idx] = (int)my_radius;
        points_xy_image[idx] = point_image;
        conic_opacity[idx].x = conic.x;
        conic_opacity[idx].y = conic.y;
        conic_opacity[idx].z = conic.z;
        conic_opacity[idx].w = opacities[idx];
        tiles_touched[idx] = (rect_max.y - rect_min.y) * (rect_max.x - rect_min.x);
    }
}

void renderCPU(
    const dim3 grid,
    const uint2 *ranges,
    const uint32_t *point_list,
    int W, int H,
    const float2 *points_xy_image,
    const float *features,
    const float4 *conic_opacity,
    float *final_T,
    uint32_t *n_contrib,
    const float *bg_color,
    float *out_color)
{
    uint32_t vertical_blocks = (H + BLOCK_Y - 1) / BLOCK_Y;
	uint32_t horizontal_blocks = (W + BLOCK_X - 1) / BLOCK_X;
    // Loop over each tile
    for (unsigned int tile_y = 0; tile_y < vertical_blocks; tile_y++)
    {
        for (unsigned int tile_x = 0; tile_x < horizontal_blocks; tile_x++)
        {
            uint2 pix_min = {tile_x * BLOCK_X, tile_y * BLOCK_Y};
            uint2 pix_max = {min_int(pix_min.x + BLOCK_X, W), min_int(pix_min.y + BLOCK_Y, H)};

            uint2 range = ranges[tile_y * horizontal_blocks + tile_x];
            int toDo = range.y - range.x;

            // For each pixel in the tile
            for (unsigned int pix_y = pix_min.y; pix_y < pix_max.y; pix_y++)
            {
                for (unsigned int pix_x = pix_min.x; pix_x < pix_max.x; pix_x++)
                {
                    uint32_t pix_id = W * pix_y + pix_x;
                    float2 pixf = {(float)pix_x, (float)pix_y};

                    float T = 1.0f;
                    uint32_t contributor = 0;
                    uint32_t last_contributor = 0;
                    float C[NUM_CHANNELS] = {0.0f};

                    // For each Gaussian in the tile
                    for (int idx = range.x; idx < range.y; idx++)
                    {
                        contributor++;
                        int coll_id = point_list[idx];

                        float2 xy = points_xy_image[coll_id];
                        float2 d = {xy.x - pixf.x, xy.y - pixf.y};
                        float4 con_o = conic_opacity[coll_id];
                        float power = -0.5f * (con_o.x * d.x * d.x + con_o.z * d.y * d.y) - con_o.y * d.x * d.y;
                        if (power > 0.0f)
                            continue;

                        float alpha = fmin(0.99f, con_o.w * exp(power));
                        if (alpha < 1.0f / 255.0f)
                            continue;
                        float test_T = T * (1 - alpha);
                        if (test_T < 0.0001f)
                            break;

                        for (int ch = 0; ch < NUM_CHANNELS; ch++)
                            C[ch] += features[coll_id * NUM_CHANNELS + ch] * alpha * T;

                        T = test_T;
                        last_contributor = contributor;
                    }

                    final_T[pix_id] = T;
                    n_contrib[pix_id] = last_contributor;
                    for (int ch = 0; ch < NUM_CHANNELS; ch++)
                        out_color[ch * H * W + pix_id] = C[ch] + T * bg_color[ch];
                }
            }
        }
    }
}

void cpu_rasterizer_render(
    const dim3 grid,
    const uint2 *ranges,
    const uint32_t *point_list,
    int W, int H,
    const float2 *means2D,
    const float *colors,
    const float4 *conic_opacity,
    float *final_T,
    uint32_t *n_contrib,
    const float *bg_color,
    float *out_color)
{
    // Call the CPU version of the render function
    renderCPU(
        grid,
        ranges,
        point_list,
        W, H,
        means2D,
        colors,
        conic_opacity,
        final_T,
        n_contrib,
        bg_color,
        out_color);
}

void cpu_rasterizer_preprocess(int P, int D, int M,
                const float *means3D,
                const float3 *scales,
                const float scale_modifier,
                const float4 *rotations,
                const float *opacities,
                const float *shs,
                int *clamped,
                const float *cov3D_precomp,
                const float *colors_precomp,
                const float *viewmatrix,
                const float *projmatrix,
                const float3 *cam_pos,
                const int W, int H,
                const float tan_fovx, float tan_fovy,
                const float focal_x, const float focal_y,
                int *radii,
                float2 *means2D,
                float *depths,
                float *cov3Ds,
                float *rgb,
                float4 *conic_opacity,
                const dim3 grid,
                uint32_t *tiles_touched,
                int prefiltered)
{
    preprocessCPU(
		P, D, M,
		means3D,
		scales,
		scale_modifier,
		rotations,
		opacities,
		shs,
		clamped,
		cov3D_precomp,
		colors_precomp,
		viewmatrix, 
		projmatrix,
		cam_pos,
		W, H,
		tan_fovx, tan_fovy,
		focal_x, focal_y,
		radii,
		means2D,
		depths,
		cov3Ds,
		rgb,
		conic_opacity,
		grid,
		tiles_touched,
		prefiltered
		);
}
