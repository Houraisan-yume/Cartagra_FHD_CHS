#include "OverlayLyric.h"
#include "GraphicsRoundRectPath.hpp"
#include "Utils.hpp"
#include <cmath>

#define IDT_MOUSETRAP 100001

BOOL OverlayLyric::InitializeGdiplus() {
	if (mGdiplusToken == NULL) {
		::GdiplusStartupInput GpInput;
		GpInput.GdiplusVersion = 1;
		if (::GdiplusStartup(&mGdiplusToken, &GpInput, NULL)) {
			return FALSE;
		}
	}
	return TRUE;
}

void OverlayLyric::ReleaseGdiplus() {
	if (mGdiplusToken != NULL) {
		::GdiplusShutdown(mGdiplusToken);
	}
}

OverlayLyric::OverlayLyric(HINSTANCE hInst) : Wnd(hInst) {
	// ��ʼ��Ĭ�ϲ���
	wcscpy_s(mFontInfo.FontName, L"����");
	mFontInfo.FontSize = 42;
	mFontInfo.ForeColor1 = 0xFF013C8F;
	mFontInfo.ForeColor2 = 0xFF0198D4;
	mFontInfo.LineColor = 0x90000000;
	mFontInfo.OkColor1 = 0xFFBCF9FC;
	mFontInfo.OkColor2 = 0xFF67F0FC;

	this->InitializeGdiplus();  // ��ʼ��GDI+
	this->Init();
}

OverlayLyric::~OverlayLyric() {
	HWND hWnd = GetHandle();
	KillTimer(hWnd, IDT_MOUSETRAP);
	this->ReleaseGdiplus(); // �ͷ�GDI+
}

BOOL OverlayLyric::DrawBackground(::Graphics* const graphics, const SIZE& size) {
	::Bitmap bitmap(size.cx, size.cy, PixelFormat32bppARGB);
	::Graphics bitmapGraphics(&bitmap);

	const REAL shadowSize = 10.f;

	GraphicsRoundRectPath path;
	path.AddRoundRect(0, 0, size.cx, size.cy, 5, 5);
	path.CloseFigure();
	::PathGradientBrush pthGrBrush(&path);
	pthGrBrush.SetGammaCorrection(TRUE);
	::Color colors[] = { ::Color::Transparent };
	INT num = 1;
	pthGrBrush.SetSurroundColors(colors, &num);
	pthGrBrush.SetCenterColor(::Color(128, 0, 0, 0));
	pthGrBrush.SetFocusScales(1.f - shadowSize / REAL(size.cx), 1.f - shadowSize / REAL(size.cy));
	bitmapGraphics.FillPath(&pthGrBrush, &path);
	graphics->DrawImage(&bitmap, 0, 0, size.cx, size.cy);

	return TRUE;
}

BOOL OverlayLyric::DrawText(::Graphics* const graphics, const SIZE& size)
{
	int textLen = lstrlenW(mText);
	if (textLen == 0) {
		// ����ַ�������Ϊ0�򲻻���
		return TRUE;
	}
	// �������ֳ��Ⱥ�����size������һ����Щ���ֿ�����Ҫ��size
	INT boardWidth = INT(ceil(mFontInfo.FontSize * textLen + mFontInfo.FontSize * 2.f));
	INT boardHeight = INT(ceil(mFontInfo.FontSize * 2.f));
	// ������ʱλͼ
	::Bitmap bitmap(boardWidth, boardHeight, PixelFormat32bppARGB);
	::Graphics bitmapGraphics(&bitmap);
	bitmapGraphics.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeHighQuality);
	bitmapGraphics.SetInterpolationMode(Gdiplus::InterpolationMode::InterpolationModeBilinear);

	::StringFormat format;
	format.SetAlignment(::StringAlignmentNear);
	::FontFamily fontFamily(mFontInfo.FontName);
	::GraphicsPath path;
	::RectF rclayout(0.f, 0.f, REAL(boardWidth), REAL(boardHeight));

	// ����������Ӱ�����������
	const REAL offset[6] = { 1.f, 1.5f, 2.f, 2.2f, -1.f, -1.5f };
	const DWORD colors[6] = { 0x90000000, 0x30000000, 0x20000000, 0x10000000, 0x40000000, 0x30000000 };
	const int layerNum = 6;
	for (int i = 0; i < layerNum; i++) {
		rclayout.X = offset[i];
		rclayout.Y = offset[i];
		path.AddString(mText, -1, &fontFamily, ::FontStyleBold, mFontInfo.FontSize, rclayout, &format);
		::Color color(colors[i]);
		::SolidBrush brush(color);  // ::LinearGradientBrush brush(brushRect, color, color, ::LinearGradientModeVertical);
		bitmapGraphics.FillPath(&brush, &path);
		path.Reset();
	}
	rclayout.X = 0.f;
	rclayout.Y = 0.f;
	path.AddString(mText, -1, &fontFamily, ::FontStyleBold, mFontInfo.FontSize, rclayout, &format);
	// ���һ�������ˢ
	::RectF brushRect(0, 0, mFontInfo.FontSize, mFontInfo.FontSize);
	::LinearGradientBrush brush(brushRect, ::Color(mFontInfo.ForeColor1), ::Color(mFontInfo.ForeColor2), ::LinearGradientModeVertical);
	bitmapGraphics.FillPath(&brush, &path);
	
	// ������Ƶ����ַ�Χ
	::RectF rcBound;
	path.GetBounds(&rcBound);
	// ��ʼ���ƿ���OK
	if (mScale > 0.f) {
		::Bitmap okBitmap(boardWidth, boardHeight, PixelFormat32bppARGB);
		::Graphics okGraphics(&okBitmap);
		okGraphics.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeHighQuality);
		okGraphics.SetInterpolationMode(Gdiplus::InterpolationMode::InterpolationModeBilinear);
		::LinearGradientBrush okBrush(brushRect, ::Color(mFontInfo.OkColor1), ::Color(mFontInfo.OkColor2), ::LinearGradientModeVertical);
		okGraphics.FillPath(&okBrush, &path);
		okGraphics.Flush();
		bitmapGraphics.DrawImage(&okBitmap, 0.f, 0.f, 0.f, 0.f, rcBound.GetRight() * mScale, rcBound.GetBottom(), ::UnitPixel);  // ������OK���Ƶ���λͼ��
	}
	// ����һ����ߵı�ˢ
	::Pen pen(::Color(mFontInfo.LineColor));
	bitmapGraphics.DrawPath(&pen, &path);
	bitmapGraphics.Flush();

	// �ü���������
	rcBound.X = rcBound.X - 1.5f;
	//rcBound.Y = rcBound.Y - 1.5f;
	rcBound.Width = rcBound.Width + 3.7f;
	rcBound.Height = rcBound.Height + 3.f;
	Bitmap* clone = bitmap.Clone(rcBound, PixelFormat32bppARGB);
	// ���Ƶ���graphics��
	graphics->DrawImage(clone, size.cx / 2 - rcBound.Width / 2, size.cy / 2 - rcBound.Height / 2);
	delete clone; // �ͷ��ڴ�

	// graphics->DrawImage(&bitmap, size.cx / 2 - boardWidth / 2, size.cy / 2 - boardHeight / 2);

	return TRUE;
}

void OverlayLyric::SetText(LPCWCHAR text)
{
	wcscpy_s(mText, text);
}

void OverlayLyric::SetScale(const REAL& scale)
{
	mScale = scale;
	if (mScale < 0.f) {
		mScale = 0.f;
	}
	else if (mScale > 1.f) {
		mScale = 1.f;
	}
}

void OverlayLyric::SetFont(LPCWCHAR fontName, const REAL& fontSize)
{
	wcscpy_s(mFontInfo.FontName, fontName);
	mFontInfo.FontSize = fontSize;
}

void OverlayLyric::SetDefaultStyle(DefaultColorStyle colorStyle)
{
	switch (colorStyle) {
	case DefaultColorStyle::Blue:
		mFontInfo.ForeColor1 = 0xFF013C8F;
		mFontInfo.ForeColor2 = 0xFF0198D4;
		mFontInfo.OkColor1 = 0xFFBCF9FC;
		mFontInfo.OkColor2 = 0xFF67F0FC;
		break;
	case DefaultColorStyle::Green:
		mFontInfo.ForeColor1 = 0xFF87F321;
		mFontInfo.ForeColor2 = 0xFF0E6700;
		mFontInfo.OkColor1 = 0xFFDCFEAE;
		mFontInfo.OkColor2 = 0xFFE4FE04;
		break;
	case DefaultColorStyle::Red:
		mFontInfo.ForeColor1 = 0xFFFECEFC;
		mFontInfo.ForeColor2 = 0xFFE144CD;
		mFontInfo.OkColor1 = 0xFFFEFE65;
		mFontInfo.OkColor2 = 0xFFFEA10F;
		break;
	case DefaultColorStyle::White:
		mFontInfo.ForeColor1 = 0xFFFBFBFA;
		mFontInfo.ForeColor2 = 0xFFCBCBCB;
		mFontInfo.OkColor1 = 0xFF62DDFF;
		mFontInfo.OkColor2 = 0xFF229CFE;
		break;
	case DefaultColorStyle::Yellow:
		mFontInfo.ForeColor1 = 0xFFFE7A00;
		mFontInfo.ForeColor2 = 0xFFFF0000;
		mFontInfo.OkColor1 = 0xFFFFFF6E;
		mFontInfo.OkColor2 = 0xFFFEA10F;
		break;
	default:
		mFontInfo.ForeColor1 = 0xFF013C8F;
		mFontInfo.ForeColor2 = 0xFF0198D4;
		mFontInfo.OkColor1 = 0xFFBA0000;
		mFontInfo.OkColor2 = 0xFFFF9301;
		break;
	}
}

void OverlayLyric::SetWndPos(INT x, INT y, INT width, INT height)
{
	HWND hWnd = GetHandle();
	MoveWindow(hWnd, x, y, width, height, FALSE);
}

BOOL OverlayLyric::Update()
{
	HWND hWnd = GetHandle();

	SIZE winSize = { 0 };
	Utils::GetClientSize(hWnd, &winSize);

	HDC hDC = GetDC(hWnd);
	BITMAPINFO tempBI;
	memset(&tempBI, 0, sizeof(BITMAPINFO));
	tempBI.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	tempBI.bmiHeader.biBitCount = 32;
	tempBI.bmiHeader.biHeight = winSize.cy;
	tempBI.bmiHeader.biWidth = winSize.cx;
	tempBI.bmiHeader.biPlanes = 1;
	tempBI.bmiHeader.biCompression = BI_RGB;
	tempBI.bmiHeader.biSizeImage = tempBI.bmiHeader.biWidth * tempBI.bmiHeader.biHeight * tempBI.bmiHeader.biBitCount / 8;

	HDC hMemDC = CreateCompatibleDC(hDC);
	PVOID pBmpData = NULL;
	HBITMAP hBitmap = CreateDIBSection(hMemDC, &tempBI, DIB_RGB_COLORS, &pBmpData, NULL, NULL);
	if (hBitmap == NULL) {
		// �����豸�޹�λͼʧ���򷵻�FALSE
		DeleteDC(hMemDC);
		ReleaseDC(hWnd, hDC);
		return FALSE;
	}
	HBITMAP hBitmapOld = (HBITMAP)SelectObject(hMemDC, hBitmap);  // �µĶ������ͬһ���͵��϶��󣬷����϶���

	::Graphics graphics(hMemDC);
	graphics.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeHighQuality);
	graphics.SetInterpolationMode(Gdiplus::InterpolationMode::InterpolationModeBilinear);

	if (mShowBackground) {
		DrawBackground(&graphics, winSize);
	}
	//DrawText(&graphics, winSize);
	/*Bitmap bitmap1(LR"(.\\CHSPAK\\THANKS.png)");
	int originalWidth = bitmap1.GetWidth();
	int originalHeight = bitmap1.GetHeight();

	Bitmap scaledBitmap1(newWidth, newHeight, bitmap.GetPixelFormat());
	Graphics graphics(&scaledBitmap1);
	graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	graphics.DrawImage(&bitmap, 0, 0, newWidth, newHeight);*/
	 
	graphics.DrawImage(Bitmap::FromFile(LR"(.\\CHSPAK\\THANKS.png)"),0,0, winSize.cx, winSize.cy);

	::POINT srcPoint = { 0, 0 };
	BLENDFUNCTION blendFunc32bpp = { 0 };
	blendFunc32bpp.BlendOp = AC_SRC_OVER;
	blendFunc32bpp.BlendFlags = 0;
	blendFunc32bpp.SourceConstantAlpha = 255;
	blendFunc32bpp.AlphaFormat = AC_SRC_ALPHA;
	UpdateLayeredWindow(hWnd, hDC, NULL, &winSize, hMemDC, &srcPoint, 0, &blendFunc32bpp, ULW_ALPHA);

	SelectObject(hMemDC, hBitmapOld);
	DeleteObject(hBitmap);
	DeleteDC(hMemDC);
	ReleaseDC(hWnd, hDC);
	return TRUE;
}

void OverlayLyric::SetLock(BOOL lock)
{
	HWND hWnd = GetHandle();
	LONG ret = GetWindowLong(hWnd, GWL_EXSTYLE);
	if (lock) {
		ret |= WS_EX_TRANSPARENT;
	}
	else {
		ret ^= WS_EX_TRANSPARENT;
	}
	SetWindowLong(hWnd, GWL_EXSTYLE, ret);
}

void OverlayLyric::ShowWnd()
{
	this->Show();
}

void OverlayLyric::HideWnd()
{
	this->Hide();
}
#include<mutex>
LRESULT OverlayLyric::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hWnd = GetHandle();
	static std::mutex _m;
	static bool hided = false;
	auto hideandnotify = [this]() {
		std::lock_guard<std::mutex> _(_m);
		if (hided)return;
		hided = true;
		HideWnd();
		SECURITY_DESCRIPTOR sd;
		InitializeSecurityDescriptor(&(sd), 1);
		SetSecurityDescriptorDacl(&(sd), 1, 0, 0);
		SECURITY_ATTRIBUTES allacc;
		allacc.nLength = sizeof(allacc);
		allacc.bInheritHandle = 0;
		allacc.lpSecurityDescriptor = &(sd);
		auto event=CreateEventW(&allacc, 0, 0, L"LIANYUYUEKUANGBING_SHOW_THANKS");
		SetEvent(event);
		CloseHandle(event);
	};
	switch (message)
	{
	case WM_CREATE:
	{
		/// | WS_EX_TRANSPARENT
		SetWindowLong(hWnd, GWL_EXSTYLE,WS_EX_NOACTIVATE| WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST);  // ���ô�����չ��ʽ

		/* ��ʼ������Ĭ��λ�� */
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYFULLSCREEN);
		int cx = int(float(screenWidth) * 0.8f);
		int cy = int(mFontInfo.FontSize * 2.0f);
		int x = screenWidth / 2 - cx / 2;
		int y = screenHeight - cy;
		MoveWindow(hWnd, x, y, cx, cy, FALSE);
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		SetTimer(hWnd, IDT_MOUSETRAP, 100, NULL);
		std::thread([hideandnotify]() {
			
			Sleep(10000);
			hideandnotify();

			}).detach();
	}
	break;
	case WM_KEYDOWN: {
		if (wParam == VK_RETURN || wParam == VK_CONTROL || wParam == VK_SHIFT) {
			hideandnotify();
		} 
		break;
	}

	case WM_RBUTTONDOWN:  //��갴��
	case WM_LBUTTONDOWN:  //��갴��
	{
		SetCapture(hWnd);	//��ռ�����Ϣ
		mMouseXY.y = HIWORD(lParam);
		mMouseXY.x = LOWORD(lParam);
		mIsMousePrees = TRUE;
		hideandnotify();
	}
	break;
	case WM_LBUTTONUP:   //����ɿ�
	{
		ReleaseCapture();  //�ͷŶ�ռ�����Ϣ
		mIsMousePrees = FALSE;
	}
	break;
	 
	case WM_ACTIVATE: 
	{
		// ���ڼ���ʱ������ǰ����
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	break;
	case WM_DISPLAYCHANGE:
	{
		int width = LOWORD(lParam);    // p & 0xFFFF;
		int height = HIWORD(lParam);   // >> 16;
		// TODO �����ֱ��ʸı��¼�
	}
	break;
	case WM_CLOSE: {
		hideandnotify();
	}
	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}