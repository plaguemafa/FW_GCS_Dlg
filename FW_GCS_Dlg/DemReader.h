// DemReader.h: 基于 GDAL 读取 DEM（如 SRTM VRT）并在给定经纬度返回高程（米）
#pragma once

// 使用 C 链接，避免头文件暴露 GDAL，调用方无需包含 GDAL
#ifdef __cplusplus
extern "C" {
#endif

// 打开 DEM 数据集（支持 .vrt、.tif 等 GDAL 可读格式）
// 路径为 UTF-8；若 path 为 NULL 或空，则使用默认路径（程序目录下的 SRTM_DEM\\terrain.vrt）
// 返回 true 表示打开成功
int DemReaderOpen(const char* path);

// 关闭当前打开的 DEM，释放资源
void DemReaderClose(void);

// 在给定经纬度（度）处查询高程（米）
// 返回 1 表示成功，*out_elevation_m 为有效高程；返回 0 表示无数据或未打开 DEM，*out_elevation_m 不变
int DemReaderGetElevation(double lon_deg, double lat_deg, double* out_elevation_m);

#ifdef __cplusplus
}
#endif
