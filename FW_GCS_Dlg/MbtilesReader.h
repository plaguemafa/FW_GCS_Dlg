#pragma once

#include <vector>
#include <string>
#include <windows.h>
#include <afx.h>

struct MbtilesMetadata
{
	int minZoom = 0;
	int maxZoom = 18;
	int defaultZoom = 10;
	double centerLat = 0.0;
	double centerLng = 0.0;
	CString format = _T("png");
	bool hasCenter = false;
	// Bounds: [minLng, minLat, maxLng, maxLat]
	double minLng = -180.0;
	double minLat = -90.0;
	double maxLng = 180.0;
	double maxLat = 90.0;
	bool hasBounds = false;
};

class CMbtilesReader
{
public:
	CMbtilesReader();
	~CMbtilesReader();

	bool Open(const CString& path, CString& errorMessage);
	void Close();
	bool IsOpen() const;

	bool GetTile(int zoom, int x, int y, std::vector<unsigned char>& outData, CString& outMimeType, bool& outIsGzip);
	bool GetMetadata(MbtilesMetadata& outMetadata) const;

private:
	struct sqlite3;
	struct sqlite3_stmt;

	struct SqliteApi
	{
		HMODULE dll = nullptr; //dll句柄
		int (*open_v2)(const char*, sqlite3**, int, const char*) = nullptr; //打开数据库
		int (*close)(sqlite3*) = nullptr; //关闭数据库
		int (*prepare_v2)(sqlite3*, const char*, int, sqlite3_stmt**, const char**) = nullptr; //准备语句
		int (*bind_int)(sqlite3_stmt*, int, int) = nullptr; //绑定整数
		int (*step)(sqlite3_stmt*) = nullptr; //执行语句
		const void* (*column_blob)(sqlite3_stmt*, int) = nullptr; //获取二进制数据
		int (*column_bytes)(sqlite3_stmt*, int) = nullptr; //获取数据长度
		int (*column_int)(sqlite3_stmt*, int) = nullptr; //获取整数
		const unsigned char* (*column_text)(sqlite3_stmt*, int) = nullptr; //获取文本数据
		int (*finalize)(sqlite3_stmt*) = nullptr; //释放语句
		const char* (*errmsg)(sqlite3*) = nullptr; //获取错误信息
	};

	bool LoadSqliteApi(CString& errorMessage); //加载sqlite3.dll
	void UnloadSqliteApi(); //卸载sqlite3.dll
	bool LoadMetadata(CString& errorMessage); //加载元数据
	bool QueryMetadataValue(const char* key, std::string& outValue) const; //查询元数据值
	static std::string WideToUtf8(const CString& value); //将宽字符串转换为UTF-8字符串

	SqliteApi m_api;
	sqlite3* m_db; //数据库句柄
	CString m_openPath; //打开路径
	MbtilesMetadata m_metadata;
};
