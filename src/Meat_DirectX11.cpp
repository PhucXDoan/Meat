#include <windows.h>
#include <windowsx.h>
#include <dxgi.h>
#include <d3d11.h>
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
	//
	// Window initialization.
	//

	WNDCLASSEXA window_info = {};
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

	//
	// DirectX11 Initialization.
	//

	IDXGISwapChain*      swapchain;
	ID3D11Device*        device;
	ID3D11DeviceContext* device_context;

	{
		DXGI_SWAP_CHAIN_DESC swapchain_description = {};
		swapchain_description.BufferCount       = 1;
		swapchain_description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchain_description.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchain_description.OutputWindow      = window_handle;
		swapchain_description.SampleDesc.Count  = 1;
		swapchain_description.Windowed          = true;

		constexpr UINT DEVICE_FLAGS =
			#if DEBUG
				D3D11_CREATE_DEVICE_DEBUG;
			#else
				0;
			#endif

		if (D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, DEVICE_FLAGS, 0, 0, D3D11_SDK_VERSION, &swapchain_description, &swapchain, &device, 0, &device_context) != S_OK)
		{
			ASSERT(!"Failed to initialize DirectX11.");
			return -1;
		}
	}
	DEFER
	{
		swapchain->Release();
		device->Release();
		device_context->Release();
	};

	//
	// Set render target.
	//

	ID3D11RenderTargetView* target;
	{
		ID3D11Texture2D* texture;
		swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&texture));
		DEFER { texture->Release(); };
		device->CreateRenderTargetView(texture, 0, &target);
	}
	DEFER { target->Release(); };
	device_context->OMSetRenderTargets(1, &target, 0);

	//
	// Set viewport.
	//

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width    = 800;
	viewport.Height   = 600;
	device_context->RSSetViewports(1, &viewport);

	//
	// Loop.
	//

	ShowWindow(window_handle, cmd_show);

	constexpr f32 SECONDS_PER_FRAME = 1.0f / 24.0f;

	f32           frame_time = 0.0f;
	LARGE_INTEGER counter_start;
	LARGE_INTEGER counter_end;
	LARGE_INTEGER counter_frequency;
	QueryPerformanceFrequency(&counter_frequency);
	QueryPerformanceCounter(&counter_start);

	while (true)
	{
		QueryPerformanceCounter(&counter_end);
		f32 delta_time = (counter_end.QuadPart - counter_start.QuadPart) / static_cast<f32>(counter_frequency.QuadPart);
		QueryPerformanceCounter(&counter_start);

		frame_time += delta_time;

		for (MSG message; PeekMessageA(&message, 0, 0, 0, PM_REMOVE);)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);

			if (message.message == WM_QUIT)
			{
				goto BREAK;
			}
		}

		if (frame_time >= SECONDS_PER_FRAME)
		{
			f32 bg_color[4] = {};

			do
			{
				persist f32 TIME = 0;
				TIME += SECONDS_PER_FRAME;

				FOR_ELEMS(bg_color)
				{
					*it = clamp(powf(cosf(TAU / 2.0f * TIME), 40.0f), 0.0f, 1.0f);
				}

				frame_time -= SECONDS_PER_FRAME;
			}
			while (frame_time >= SECONDS_PER_FRAME);

			device_context->ClearRenderTargetView(target, bg_color);

			swapchain->Present(0, 0);
		}

		Sleep(1); // @TODO@ Cools the CPU. Should be better.
	}
	BREAK:;

	return 0;
}
