#include "pch.h"
#include "MbtilesReader.h"
#include <cstdlib>
#include <cmath>

namespace
{
	const int SQLITE_OK = 0;
	const int SQLITE_ROW = 100;
	const int SQLITE_DONE = 101;
	const int SQLITE_OPEN_READONLY = 0x00000001;
	const int SQLITE_OPEN_NOMUTEX = 0x00008000;
	const int SQLITE_OPEN_PRIVATECACHE = 0x00040000;

	bool ParseDoubleList(const std::string& text, double* values, int count)
	{
		if (values == nullptr || count <= 0)
		{
			return false;
		}
		const char* start = text.c_str();
		char* end = nullptr;
		for (int i = 0; i < count; ++i)
		{
			values[i] = std::strtod(start, &end);
			if (end == start)
			{
				return false;
			}
			if (*end == ',')
			{
				start = end + 1;
			}
			else
			{
				start = end;
			}
		}
		return true;
	}
}

CMbtilesReader::CMbtilesReader()
	: m_db(nullptr)
{
}

CMbtilesReader::~CMbtilesReader()
{
	Close();
}

bool CMbtilesReader::Open(const CString& path, CString& errorMessage)
{
	Close();
	if (path.IsEmpty())
	{
		errorMessage = _T("未提供mbtiles路径");
		return false;
	}
	if (!LoadSqliteApi(errorMessage))
	{
		return false;
	}

	std::string utf8Path = WideToUtf8(path);
	int rc = m_api.open_v2(utf8Path.c_str(), &m_db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_PRIVATECACHE, nullptr);
	if (rc != SQLITE_OK || m_db == nullptr)
	{
		CString err = _T("sqlite3_open_v2 失败");
		if (m_api.errmsg && m_db != nullptr)
		{
			err.Format(_T("sqlite3_open_v2 失败: %S"), m_api.errmsg(m_db));
		}
		errorMessage = err;
		Close();
		return false;
	}

	m_openPath = path;
	if (!LoadMetadata(errorMessage))
	{
		// 允许打开成功但元数据不完整
		TRACE(_T("MbtilesReader: metadata读取失败: %s\n"), errorMessage.GetString());
	}
	return true;
}

void CMbtilesReader::Close()
{
	if (m_db != nullptr && m_api.close != nullptr)
	{
		m_api.close(m_db);
	}
	m_db = nullptr;
	m_openPath.Empty();
	UnloadSqliteApi();
}

bool CMbtilesReader::IsOpen() const
{
	return m_db != nullptr;
}

bool CMbtilesReader::GetTile(int zoom, int x, int y, std::vector<unsigned char>& outData, CString& outMimeType, bool& outIsGzip)
{
	outData.clear();
	outMimeType.Empty();
	outIsGzip = false;
	if (m_db == nullptr)
	{
		return false;
	}

	// MBTiles使用TMS坐标系，需要翻转Y轴
	const int maxIndex = 1 << zoom;
	const int tmsY = (maxIndex - 1) - y;

	const char* sql = "SELECT tile_data FROM tiles WHERE zoom_level=? AND tile_column=? AND tile_row=?;";
	sqlite3_stmt* stmt = nullptr;
	const char* tail = nullptr;
	int rc = m_api.prepare_v2(m_db, sql, -1, &stmt, &tail);
	if (rc != SQLITE_OK || stmt == nullptr)
	{
		return false;
	}

	m_api.bind_int(stmt, 1, zoom);
	m_api.bind_int(stmt, 2, x);
	m_api.bind_int(stmt, 3, tmsY);

	rc = m_api.step(stmt);
	if (rc == SQLITE_ROW)
	{
		const void* blob = m_api.column_blob(stmt, 0);
		int size = m_api.column_bytes(stmt, 0);
		if (blob != nullptr && size > 0)
		{
			outData.assign(reinterpret_cast<const unsigned char*>(blob), reinterpret_cast<const unsigned char*>(blob) + size);
			if (m_metadata.format.CompareNoCase(_T("jpg")) == 0 || m_metadata.format.CompareNoCase(_T("jpeg")) == 0)
			{
				outMimeType = _T("image/jpeg");
			}
			else if (m_metadata.format.CompareNoCase(_T("pbf")) == 0)
			{
				outMimeType = _T("application/x-protobuf");
				if (size >= 2)
				{
					outIsGzip = (outData[0] == 0x1F && outData[1] == 0x8B);
				}
			}
			else
			{
				outMimeType = _T("image/png");
			}
		}
	}

	m_api.finalize(stmt);
	return !outData.empty();
}

bool CMbtilesReader::GetMetadata(MbtilesMetadata& outMetadata) const
{
	if (m_db == nullptr)
	{
		return false;
	}
	outMetadata = m_metadata;
	return true;
}

bool CMbtilesReader::LoadSqliteApi(CString& errorMessage)
{
	if (m_api.dll != nullptr)
	{
		return true;
	}

	wchar_t exePath[MAX_PATH] = {};
	::GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	CString exeDir(exePath);
	int pos = exeDir.ReverseFind(L'\\');
	if (pos >= 0)
	{
		exeDir = exeDir.Left(pos);
	}

	CString dllPathLower = exeDir + _T("\\sqlite3.dll");
	CString dllPathUpper = exeDir + _T("\\SQLite3.dll");

	m_api.dll = ::LoadLibraryW(dllPathLower);
	if (m_api.dll == nullptr)
	{
		m_api.dll = ::LoadLibraryW(dllPathUpper);
	}
	if (m_api.dll == nullptr)
	{
		m_api.dll = ::LoadLibraryW(L"sqlite3.dll");
	}
	if (m_api.dll == nullptr)
	{
		errorMessage = _T("sqlite3.dll not found (place near exe or in PATH)");
		return false;
	}

	m_api.open_v2 = reinterpret_cast<decltype(m_api.open_v2)>(::GetProcAddress(m_api.dll, "sqlite3_open_v2"));
	m_api.close = reinterpret_cast<decltype(m_api.close)>(::GetProcAddress(m_api.dll, "sqlite3_close"));
	m_api.prepare_v2 = reinterpret_cast<decltype(m_api.prepare_v2)>(::GetProcAddress(m_api.dll, "sqlite3_prepare_v2"));
	m_api.bind_int = reinterpret_cast<decltype(m_api.bind_int)>(::GetProcAddress(m_api.dll, "sqlite3_bind_int"));
	m_api.step = reinterpret_cast<decltype(m_api.step)>(::GetProcAddress(m_api.dll, "sqlite3_step"));
	m_api.column_blob = reinterpret_cast<decltype(m_api.column_blob)>(::GetProcAddress(m_api.dll, "sqlite3_column_blob"));
	m_api.column_bytes = reinterpret_cast<decltype(m_api.column_bytes)>(::GetProcAddress(m_api.dll, "sqlite3_column_bytes"));
	m_api.column_text = reinterpret_cast<decltype(m_api.column_text)>(::GetProcAddress(m_api.dll, "sqlite3_column_text"));
	m_api.finalize = reinterpret_cast<decltype(m_api.finalize)>(::GetProcAddress(m_api.dll, "sqlite3_finalize"));
	m_api.errmsg = reinterpret_cast<decltype(m_api.errmsg)>(::GetProcAddress(m_api.dll, "sqlite3_errmsg"));

	if (m_api.open_v2 == nullptr || m_api.close == nullptr || m_api.prepare_v2 == nullptr ||
		m_api.bind_int == nullptr || m_api.step == nullptr || m_api.column_blob == nullptr ||
		m_api.column_bytes == nullptr || m_api.column_text == nullptr || m_api.finalize == nullptr)
	{
		errorMessage = _T("sqlite3.dll missing required exports");
		UnloadSqliteApi();
		return false;
	}

	return true;
}

void CMbtilesReader::UnloadSqliteApi()
{
	if (m_api.dll != nullptr)
	{
		::FreeLibrary(m_api.dll);
	}
	m_api = SqliteApi{};
}

bool CMbtilesReader::LoadMetadata(CString& errorMessage)
{
	m_metadata = MbtilesMetadata{};
	std::string value;

	if (QueryMetadataValue("format", value))
	{
		m_metadata.format = value.c_str();
	}
	if (QueryMetadataValue("minzoom", value))
	{
		m_metadata.minZoom = std::atoi(value.c_str());
	}
	if (QueryMetadataValue("maxzoom", value))
	{
		m_metadata.maxZoom = std::atoi(value.c_str());
	}
	if (QueryMetadataValue("center", value))
	{
		double center[3] = { 0.0, 0.0, 0.0 };
		if (ParseDoubleList(value, center, 3))
		{
			m_metadata.centerLng = center[0];
			m_metadata.centerLat = center[1];
			m_metadata.defaultZoom = static_cast<int>(std::round(center[2]));
			m_metadata.hasCenter = true;
		}
	}
	else if (QueryMetadataValue("bounds", value))
	{
		double bounds[4] = { 0.0, 0.0, 0.0, 0.0 };
		if (ParseDoubleList(value, bounds, 4))
		{
			m_metadata.centerLng = (bounds[0] + bounds[2]) * 0.5;
			m_metadata.centerLat = (bounds[1] + bounds[3]) * 0.5;
			m_metadata.hasCenter = true;
		}
	}

	if (m_metadata.defaultZoom < m_metadata.minZoom)
	{
		m_metadata.defaultZoom = m_metadata.minZoom;
	}
	if (m_metadata.defaultZoom > m_metadata.maxZoom)
	{
		m_metadata.defaultZoom = m_metadata.maxZoom;
	}
	return true;
}

bool CMbtilesReader::QueryMetadataValue(const char* key, std::string& outValue) const
{
	outValue.clear();
	if (m_db == nullptr)
	{
		return false;
	}

	// sqlite3_bind_text 需要可选导出，这里用拼接避免额外依赖
	std::string query = "SELECT value FROM metadata WHERE name='" + std::string(key) + "';";
	sqlite3_stmt* stmt = nullptr;
	const char* tail = nullptr;
	int rc = m_api.prepare_v2(m_db, query.c_str(), -1, &stmt, &tail);
	if (rc != SQLITE_OK || stmt == nullptr)
	{
		return false;
	}

	rc = m_api.step(stmt);
	if (rc == SQLITE_ROW)
	{
		const unsigned char* text = m_api.column_text(stmt, 0);
		if (text != nullptr)
		{
			outValue = reinterpret_cast<const char*>(text);
		}
	}
	m_api.finalize(stmt);
	return !outValue.empty();
}

std::string CMbtilesReader::WideToUtf8(const CString& value)
{
	if (value.IsEmpty())
	{
		return std::string();
	}
	CStringW wideValue(value);
	int size = ::WideCharToMultiByte(CP_UTF8, 0, wideValue.GetString(), -1, nullptr, 0, nullptr, nullptr);
	if (size <= 1)
	{
		return std::string();
	}
	std::string result;
	result.resize(static_cast<size_t>(size - 1));
	::WideCharToMultiByte(CP_UTF8, 0, wideValue.GetString(), -1, &result[0], size, nullptr, nullptr);
	return result;
}
