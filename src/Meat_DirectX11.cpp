#include <windows.h>
#include <windowsx.h>
#include "unified.h"

// @TODO@ Unicode support.

internal LRESULT CALLBACK window_callback(HWND window_handle, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		} break;

		default:
		{
			return DefWindowProcA(window_handle, message, wparam, lparam);
		} break;
	}
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int cmd_show)
{
	WNDCLASSEXA window_info = {};
	ZeroMemory(&window_info, sizeof(WNDCLASSEX)); // @TODO@ Is this really necessary?
	window_info.cbSize        = sizeof(WNDCLASSEX);
	window_info.style         = CS_HREDRAW | CS_VREDRAW;
	window_info.lpfnWndProc   = window_callback;
	window_info.hInstance     = instance;
	window_info.hCursor       = LoadCursorA(0, IDC_ARROW);
	window_info.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW); // @TODO@ Why reinterpret?
	window_info.lpszClassName = "Meat Window Class";

	if (!RegisterClassExA(&window_info))
	{
		ASSERT(!"Failed to register window class.");
		return -1;
	}

	DWORD window_style = WS_OVERLAPPEDWINDOW;
	RECT  window_rect  = { 0, 0, 500, 400 };
	if (!AdjustWindowRect(&window_rect, window_style, false))
	{
		ASSERT(!"Failed to calculate window size.");
		return -1;
	}

	HWND window_handle = CreateWindowExA(0, window_info.lpszClassName, "Meat", window_style, CW_USEDEFAULT, CW_USEDEFAULT, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top, 0, 0, instance, 0);
	if (!window_handle)
	{
		ASSERT(!"Failed to create window.");
		return -1;
	}
	DEFER { DestroyWindow(window_handle); };

	ShowWindow(window_handle, cmd_show);

	while (true)
	{
		for (MSG message; PeekMessageA(&message, 0, 0, 0, PM_REMOVE);)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);

			if (message.message == WM_QUIT)
			{
				goto BREAK;
			}
		}
	}
	BREAK:;

	return 0;
}
