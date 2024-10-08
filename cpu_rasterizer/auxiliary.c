#include<auxiliary.h>
// 判断点是否在视锥内
int cpu_rasterizer_in_frustum(int idx,
    const float* orig_points,
    const float* viewmatrix,
    const float* projmatrix,
    int prefiltered,
    float3* p_view)
{
    float3 p_orig = { orig_points[3 * idx], orig_points[3 * idx + 1], orig_points[3 * idx + 2] };

    // 将点变换到齐次裁剪空间
    float4 p_hom = cpu_rasterizer_transformPoint4x4(p_orig, projmatrix);
    float p_w = 1.0f / (p_hom.w + 1e-7f);
    float3 p_proj = { p_hom.x * p_w, p_hom.y * p_w, p_hom.z * p_w };
    *p_view = cpu_rasterizer_transformPoint4x3(p_orig, viewmatrix);

    if (p_view->z <= 0.2f) // || ((p_proj.x < -1.3f || p_proj.x > 1.3f || p_proj.y < -1.3f || p_proj.y > 1.3f)))
    {
        if (prefiltered)
        {
            printf("Point is filtered although prefiltered is set. This shouldn't happen!\n");
            exit(EXIT_FAILURE);
        }
        return 0; // false
    }
    return 1; // true
}
