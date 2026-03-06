// DemReader.cpp: 使用 GDAL 读取 DEM 并按经纬度采样高程
#include "pch.h"
#include "DemReader.h"
#include <cstdlib>
#include <cstring>
#include <cmath>

#define CPL_SUPRESS_CPLUSPLUS
#include <gdal.h>
#include <cpl_conv.h>

static GDALDatasetH g_hDS = nullptr;
static GDALRasterBandH g_hBand = nullptr;
static double g_invGeoTransform[6];
static int g_nXSize = 0, g_nYSize = 0;
static int g_hasNoData = 0;
static double g_noDataValue = 0.0;

// 默认 DEM 路径：exe 所在文件夹下的 SRTM_DEM\terrain.vrt（Debug 即 x64\Debug\SRTM_DEM\terrain.vrt，Release 即 x64\Release\SRTM_DEM\terrain.vrt）
static void getDefaultDemPath(char* out_path, size_t out_size)
{
	out_path[0] = '\0';
	WCHAR wpath[MAX_PATH] = { 0 };
	if (GetModuleFileNameW(NULL, wpath, MAX_PATH) == 0)
		return;
	// 去掉可执行文件名，得到 exe 所在目录
	WCHAR* pLast = wpath + wcslen(wpath);
	while (pLast > wpath && *pLast != L'\\') --pLast;
	if (pLast > wpath) *pLast = L'\0';
	// 相对路径：SRTM_DEM\terrain.vrt
	wcscat_s(wpath, L"\\SRTM_DEM\\terrain.vrt");
	// 转为 UTF-8
	const int n = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, out_path, (int)out_size, NULL, NULL);
	if (n <= 0) out_path[0] = '\0';
}

int DemReaderOpen(const char* path)
{
	DemReaderClose();
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");

	char demPath[1024];
	if (path && path[0] != '\0')
	{
		strncpy_s(demPath, path, _TRUNCATE);
	}
	else
	{
		getDefaultDemPath(demPath, sizeof(demPath));
		if (demPath[0] == '\0')
			return 0;
	}

	g_hDS = GDALOpenEx(demPath, GA_ReadOnly, NULL, NULL, NULL);
	if (!g_hDS)
		return 0;

	g_hBand = GDALGetRasterBand(g_hDS, 1);
	if (!g_hBand)
	{
		GDALClose(g_hDS);
		g_hDS = nullptr;
		return 0;
	}

	double adfGeoTransform[6];
	if (GDALGetGeoTransform(g_hDS, adfGeoTransform) != CE_None)
	{
		GDALClose(g_hDS);
		g_hDS = nullptr;
		g_hBand = nullptr;
		return 0;
	}
	if (!GDALInvGeoTransform(adfGeoTransform, g_invGeoTransform))
	{
		GDALClose(g_hDS);
		g_hDS = nullptr;
		g_hBand = nullptr;
		return 0;
	}

	g_nXSize = GDALGetRasterBandXSize(g_hBand);
	g_nYSize = GDALGetRasterBandYSize(g_hBand);
	g_hasNoData = 0;
	g_noDataValue = GDALGetRasterNoDataValue(g_hBand, &g_hasNoData);

	return 1;
}

void DemReaderClose(void)
{
	g_hBand = nullptr;
	if (g_hDS)
	{
		GDALClose(g_hDS);
		g_hDS = nullptr;
	}
	g_nXSize = g_nYSize = 0;
}

int DemReaderGetElevation(double lon_deg, double lat_deg, double* out_elevation_m)
{
	if (!out_elevation_m)
		return 0;
	// 首次调用时尝试打开默认 DEM 路径
	if (!g_hDS && !DemReaderOpen(nullptr))
		return 0;
	if (!g_hBand || g_nXSize <= 0 || g_nYSize <= 0)
		return 0;

	const double pixel_x = g_invGeoTransform[0] + g_invGeoTransform[1] * lon_deg + g_invGeoTransform[2] * lat_deg;
	const double pixel_y = g_invGeoTransform[3] + g_invGeoTransform[4] * lon_deg + g_invGeoTransform[5] * lat_deg;

	const int ix = (int)floor(pixel_x);
	const int iy = (int)floor(pixel_y);
	if (ix < 0 || ix >= g_nXSize || iy < 0 || iy >= g_nYSize)
		return 0;

	double value = 0.0;
	CPLErr e = GDALRasterIO(g_hBand, GF_Read, ix, iy, 1, 1, &value, 1, 1, GDT_Float64, 0, 0);
	if (e != CE_None)
		return 0;
	if (g_hasNoData && fabs(value - g_noDataValue) < 1e-10)
		return 0;

	*out_elevation_m = value;
	return 1;
}
