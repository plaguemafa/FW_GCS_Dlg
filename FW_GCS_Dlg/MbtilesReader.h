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
		HMODULE dll = nullptr;
		int (*open_v2)(const char*, sqlite3**, int, const char*) = nullptr;
		int (*close)(sqlite3*) = nullptr;
		int (*prepare_v2)(sqlite3*, const char*, int, sqlite3_stmt**, const char**) = nullptr;
		int (*bind_int)(sqlite3_stmt*, int, int) = nullptr;
		int (*step)(sqlite3_stmt*) = nullptr;
		const void* (*column_blob)(sqlite3_stmt*, int) = nullptr;
		int (*column_bytes)(sqlite3_stmt*, int) = nullptr;
		const unsigned char* (*column_text)(sqlite3_stmt*, int) = nullptr;
		int (*finalize)(sqlite3_stmt*) = nullptr;
		const char* (*errmsg)(sqlite3*) = nullptr;
	};

	bool LoadSqliteApi(CString& errorMessage);
	void UnloadSqliteApi();
	bool LoadMetadata(CString& errorMessage);
	bool QueryMetadataValue(const char* key, std::string& outValue) const;
	static std::string WideToUtf8(const CString& value);

	SqliteApi m_api;
	sqlite3* m_db;
	CString m_openPath;
	MbtilesMetadata m_metadata;
};
