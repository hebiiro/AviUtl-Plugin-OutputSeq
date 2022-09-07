#include "pch.h"
#include "OutputSeq.h"
#include "resource.h"

//--------------------------------------------------------------------

// デバッグ用コールバック関数。デバッグメッセージを出力する。
void ___outputLog(LPCTSTR text, LPCTSTR output)
{
	::OutputDebugString(output);
}

//--------------------------------------------------------------------

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}    
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

//--------------------------------------------------------------------

struct Config
{
	char m_suffix[MAX_PATH];
	int m_quality;
};

Config g_config =
{
	"%08d", 90
};

//--------------------------------------------------------------------

// コンフィグダイアログを表示する。
BOOL func_config(HWND hwnd, HINSTANCE dll_hinst)
{
	Dialog dialog(dll_hinst, MAKEINTRESOURCE(IDD_CONFIG), hwnd);

	::SetDlgItemTextA(dialog, IDC_FORMAT, g_config.m_suffix);
	::SetDlgItemInt(dialog, IDC_QUALITY, g_config.m_quality, TRUE);

	if (IDOK != dialog.doModal())
		return FALSE;

	::GetDlgItemTextA(dialog, IDC_FORMAT, g_config.m_suffix, MAX_PATH);
	g_config.m_quality = ::GetDlgItemInt(dialog, IDC_QUALITY, 0, TRUE);

	return TRUE;
}

// 設定を保存する。
int func_config_get(void* data, int size)
{
	if (data)
		memcpy(data, &g_config, sizeof(g_config));

	return sizeof(g_config);
}

// 設定を読み込む。
int func_config_set(void* data, int size)
{
	if (size != sizeof(g_config))
		return NULL;

	memcpy(&g_config, data, size);

	return size;
}

BOOL func_output(AviUtl::OutputInfo* oip)
{
	MY_TRACE(_T("func_output() begin\n"));

	LPCSTR extension = ::PathFindExtensionA(oip->savefile);
	MY_TRACE_STR(extension);

	char fileSpec[MAX_PATH] = {};
	::StringCbCopyA(fileSpec, sizeof(fileSpec), oip->savefile);
	::PathRemoveExtensionA(fileSpec);
	MY_TRACE_STR(fileSpec);

	BITMAPINFO bi = {};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = oip->w;
	bi.bmiHeader.biHeight = oip->h;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biCompression = BI_RGB;

	ULONG quality = g_config.m_quality;
	EncoderParameters encoderParameters;
	encoderParameters.Count = 1;
	encoderParameters.Parameter[0].Guid = EncoderQuality;
	encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParameters.Parameter[0].NumberOfValues = 1;
	encoderParameters.Parameter[0].Value = &quality;

	CLSID encoder;
	int result = -1;
	if (::lstrcmpiA(extension, ".bmp") == 0) result = GetEncoderClsid(L"image/bmp", &encoder);
	else if (::lstrcmpiA(extension, ".jpg") == 0) result = GetEncoderClsid(L"image/jpeg", &encoder);
	else if (::lstrcmpiA(extension, ".gif") == 0) result = GetEncoderClsid(L"image/gif", &encoder);
	else if (::lstrcmpiA(extension, ".tif") == 0) result = GetEncoderClsid(L"image/tiff", &encoder);
	else if (::lstrcmpiA(extension, ".png") == 0) result = GetEncoderClsid(L"image/png", &encoder);

	if (result == -1)
	{
		::MessageBox(0, _T("拡張子が無効です"), _T("OutputSeq"), MB_OK);

		return FALSE;
	}

	for (int i = 0; i < oip->n; i++)
	{
		if (oip->func_is_abort())
			break;

		oip->func_rest_time_disp(i, oip->n);

		void* pixelp = oip->func_get_video_ex(i, NULL);
		MY_TRACE_HEX(pixelp);

		char suffix[MAX_PATH] = {};
		::StringCbPrintfA(suffix, sizeof(suffix), g_config.m_suffix, i);
		MY_TRACE_STR(suffix);

		char fileName[MAX_PATH] = {};
		::StringCbPrintfA(fileName, sizeof(fileName), "%s%s%s", fileSpec, suffix, extension);
		MY_TRACE_STR(fileName);

		Bitmap bitmap(&bi, pixelp);

		Status status;
		if (::lstrcmpiA(extension, ".jpg") == 0)
			status = bitmap.Save((_bstr_t)fileName, &encoder, &encoderParameters);
		else
			status = bitmap.Save((_bstr_t)fileName, &encoder);
		MY_TRACE_COM_ERROR(status);

		if (status != S_OK)
		{
			::MessageBox(0, _T("ファイルの保存に失敗しました"), _T("OutputSeq"), MB_OK);

			return FALSE;
		}

		oip->func_update_preview();
	}

	MY_TRACE(_T("func_output() end\n"));

	return TRUE;
}

//--------------------------------------------------------------------

EXTERN_C AviUtl::OutputPluginDLL* WINAPI GetOutputPluginTable()
{
	static AviUtl::OutputPluginDLL output_plugin_table =
	{
		.flag = AviUtl::OutputPluginDLL::Flag::Video,
		.name = "連番出力",
		.filefilter = "BMP / JPG / GIF / TIFF / PNG File\0*.bmp;*.jpg;*.gif;*.tiff;*.png\0All File (*.*)\0*.*\0",
		.information = "連番出力 1.0.0 by 蛇色",
		.func_output = func_output,
		.func_config = func_config,
		.func_config_get = func_config_get,
		.func_config_set = func_config_set,
	};

	return &output_plugin_table;
}

//--------------------------------------------------------------------
