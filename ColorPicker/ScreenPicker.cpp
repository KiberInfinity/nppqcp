
#include <iostream>
#include <Windowsx.h>

#include "ScreenPicker.h"
#include "ColorPicker.res.h"

#define MASK_WIN_CLASS L"nplus_screen_picker"
#define INFO_WINDOW_WIDTH 180
#define INFO_WINDOW_HEIGHT 100
#define SWATCH_BG_COLOR 0x666666

ScreenPicker::ScreenPicker(){

	_instance = NULL;
	_parent_window = NULL;
	_mask_window = NULL;

	_old_color = 0;
	_new_color = 0;

}

void ScreenPicker::Create(HINSTANCE inst, HWND parent){
	
	if (IsCreated()) {
		throw std::runtime_error("ScreenPicker::Create() : duplicate creation");
	}

	if (!inst) {
		throw std::exception("ScreenPicker : instance handle required");
	}

	_instance = inst;
	_parent_window = parent;
	
	CreateMaskWindow();
	CreateInfoWindow();

}


void ScreenPicker::CreateMaskWindow(){
	
	_cursor = ::LoadCursor(_instance, MAKEINTRESOURCE(IDI_CURSOR));

	WNDCLASSEX wc = {0};
	wc.cbSize        = sizeof(wc);
	wc.lpfnWndProc   = MaskWindowWINPROC;
	wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = _instance;
	wc.hCursor       = _cursor;
	wc.lpszClassName = MASK_WIN_CLASS;

	if (!::RegisterClassEx(&wc)) {
		throw std::runtime_error("ScreenPicker: RegisterClassEx() failed");
	}

	_mask_window = ::CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, MASK_WIN_CLASS, L"", WS_POPUP, 0, 0, 0, 0, NULL, NULL, _instance, this);

	if (!_mask_window) {
		throw std::runtime_error("ScreenPicker: CreateWindowEx() function returns null");
	}

}

LRESULT CALLBACK ScreenPicker::MaskWindowWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {

	switch (message) {
		case WM_NCCREATE:
		{
			CREATESTRUCT*  cs = (CREATESTRUCT*)lparam;
			ScreenPicker* pScreenPicker = (ScreenPicker*)(cs->lpCreateParams);
			pScreenPicker->_mask_window = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pScreenPicker);
			return pScreenPicker->MaskWindowMessageHandle(message, wparam, lparam);
		}
		default:
		{
			ScreenPicker* pScreenPicker = reinterpret_cast<ScreenPicker*>(::GetWindowLongPtr(hwnd, GWL_USERDATA));
			if (!pScreenPicker)
				return FALSE;
			return pScreenPicker->MaskWindowMessageHandle(message, wparam, lparam);
		}
	}
}

BOOL ScreenPicker::MaskWindowMessageHandle(UINT message, WPARAM wparam, LPARAM lparam) {

	switch (message) {
		case WM_MOUSEMOVE:
		{
			::SetCursor(_cursor);
			SampleColor(lparam);
			return TRUE;
		}
		case WM_ACTIVATE:
		{
			return 0;
		}
		case WM_LBUTTONUP:
		{
			End();
			::SendMessage(_parent_window, WM_SCREEN_PICK_COLOR, 0, (LPARAM)_new_color);
			return TRUE;
		}
		case WM_RBUTTONUP:
		{
			End();
			::SendMessage(_parent_window, WM_SCREEN_PICK_CANCEL, 0, 0);
			return TRUE;
		}
		default:
		{
			return TRUE;
		}
	}
	
}

void ScreenPicker::Color(COLORREF color){
	_old_color = color;
}

void ScreenPicker::Start(){

	HMONITOR monitor = ::MonitorFromWindow(_parent_window, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfo(monitor, (LPMONITORINFO)&mi);
	int width = mi.rcMonitor.right - mi.rcMonitor.left;
	int height = mi.rcMonitor.bottom - mi.rcMonitor.top;

	::SetWindowPos(_mask_window, HWND_TOPMOST, mi.rcMonitor.left, mi.rcMonitor.top, width, height, SWP_SHOWWINDOW);

}

void ScreenPicker::End(){

	::SetWindowPos(_mask_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_HIDEWINDOW);
	::SetWindowPos(_info_window, HWND_TOP, 0, 0, 0, 0, SWP_HIDEWINDOW);

}

void ScreenPicker::SampleColor(LPARAM lparam){
	
	int x = GET_X_LPARAM(lparam);
	int y = GET_Y_LPARAM(lparam);

	HDC hdc = ::GetDC(HWND_DESKTOP);
	_new_color = ::GetPixel(hdc, x, y);
	::ReleaseDC(HWND_DESKTOP, hdc);

	::SendMessage(_parent_window, WM_SCREEN_HOVER_COLOR, 0, (LPARAM)_new_color);

	// place info window
	HMONITOR monitor = ::MonitorFromWindow(_parent_window, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfo(monitor, (LPMONITORINFO)&mi);

	int win_x = x+20;
	int win_y = y-20-INFO_WINDOW_HEIGHT;
	
	if(win_x + INFO_WINDOW_WIDTH > mi.rcMonitor.right)
		win_x = x-20-INFO_WINDOW_WIDTH;

	if(win_y < mi.rcMonitor.top)
		win_y = y+20;

	::SetWindowPos(_info_window, HWND_TOP, win_x, win_y, INFO_WINDOW_WIDTH, INFO_WINDOW_HEIGHT, SWP_SHOWWINDOW);

	// display color text
	wchar_t color_hex[10];
	wsprintf(color_hex, L"#%06X", _new_color);
	::SetDlgItemText(_info_window, IDC_SCR_COLOR_HEX, color_hex);

	wchar_t color_rgb[20];
	wsprintf(color_rgb, L"%d, %d, %d", GetRValue(_new_color), GetGValue(_new_color), GetBValue(_new_color));
	::SetDlgItemText(_info_window, IDC_SCR_COLOR_RGB, color_rgb);

	::SetDlgItemText(_info_window, IDC_SCR_NEW_COLOR, L"");
	::SetDlgItemText(_info_window, IDC_SCR_OLD_COLOR, L"");

}


// the information window
void ScreenPicker::CreateInfoWindow(){
			
	_info_window = ::CreateDialogParam(_instance, MAKEINTRESOURCE(IDD_SCREEN_PICKER_POPUP), NULL, (DLGPROC)InfoWindowWINPROC, (LPARAM)this);

	if(!_info_window){
		throw std::runtime_error("ScreenPicker: Create info window failed.");
	}
	
}

BOOL CALLBACK ScreenPicker::InfoWindowWINPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {

	switch (message) {
		case WM_INITDIALOG:
		{
			ScreenPicker *pScreenPicker = (ScreenPicker*)(lparam);
			pScreenPicker->_info_window = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lparam);
			pScreenPicker->InfoWindowMessageHandle(message, wparam, lparam);
			return TRUE;
		}
		default:
		{
			ScreenPicker *pScreenPicker = reinterpret_cast<ScreenPicker*>(::GetWindowLongPtr(hwnd, GWL_USERDATA));
			if (!pScreenPicker)
				return FALSE;
			return pScreenPicker->InfoWindowMessageHandle(message, wparam, lparam);
		}
	}

}


BOOL ScreenPicker::InfoWindowMessageHandle(UINT message, WPARAM wparam, LPARAM lparam) {

	switch (message) {
		case WM_INITDIALOG:
		{
			PrepareInfoWindow();
			return TRUE;
		}
		case WM_CTLCOLORSTATIC:
		{
			return OnCtlColorStatic(lparam);
		}
		default:
		{
			return FALSE;
		}
	}
	
}


void ScreenPicker::PrepareInfoWindow(){

	::SetWindowPos(_info_window, HWND_TOP, 0, 0, 0, 0, SWP_HIDEWINDOW);
	::SetWindowLong(_info_window, GWL_EXSTYLE, WS_EX_TOOLWINDOW);

	HWND ctrl = ::GetDlgItem(_info_window, IDC_SCR_COLOR_RGB);
	::SetWindowPos(ctrl, NULL, 6, INFO_WINDOW_HEIGHT-38, INFO_WINDOW_WIDTH-52, 16, SWP_NOZORDER);

	ctrl = ::GetDlgItem(_info_window, IDC_SCR_COLOR_HEX);
	::SetWindowPos(ctrl, NULL, 6, INFO_WINDOW_HEIGHT-22, INFO_WINDOW_WIDTH-52, 16, SWP_NOZORDER);

	ctrl = ::GetDlgItem(_info_window, IDC_SCR_OLD_COLOR);
	::SetWindowPos(ctrl, NULL, INFO_WINDOW_WIDTH-24, 6, 24, 16, SWP_NOZORDER);

	ctrl = ::GetDlgItem(_info_window, IDC_SCR_NEW_COLOR);
	::SetWindowPos(ctrl, NULL, INFO_WINDOW_WIDTH-48, 6, 24, 16, SWP_NOZORDER);
	

}


// WM_ONCTLCOLORSTATIC
LRESULT ScreenPicker::OnCtlColorStatic(LPARAM lparam) {

	DWORD ctrl_id = ::GetDlgCtrlID((HWND)lparam);

	switch (ctrl_id) {
		case IDC_SCR_OLD_COLOR:
		{
			if(_hbrush_old != NULL) {
				::DeleteObject(_hbrush_old);
			}
			_hbrush_old = CreateSolidBrush(_old_color);
			return (LRESULT)_hbrush_old;
		}
		case IDC_SCR_NEW_COLOR:
		{
			if(_hbrush_new != NULL) {
				::DeleteObject(_hbrush_new);
			}
			_hbrush_new = CreateSolidBrush(_new_color);
			return (LRESULT)_hbrush_new;
		}
		case IDC_COLOR_BG:
		{
			if (_hbrush_bg == NULL) {
				_hbrush_bg = CreateSolidBrush(SWATCH_BG_COLOR);
			}
			return (LRESULT)_hbrush_bg;
		}
		default:
		{
			return FALSE;
		}
	}

}