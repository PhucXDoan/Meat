// @TODO@ Unicode support.

#include <windows.h>
#include <windowsx.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include "unified.h"

constexpr f32 SECONDS_PER_FRAME = 1.0f / 24.0f;
constexpr vi2 SCREEN_DIMENSIONS = { 800, 600 };

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

	DWORD window_style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
	RECT  window_rect  = { 0, 0, SCREEN_DIMENSIONS.x, SCREEN_DIMENSIONS.y };
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
		swapchain_description.BufferDesc.Width  = SCREEN_DIMENSIONS.x;
		swapchain_description.BufferDesc.Height = SCREEN_DIMENSIONS.y;
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
		swapchain->SetFullscreenState(false, 0);
		swapchain->Release();
		device->Release();
		device_context->Release();
	};

	// @TEMP@ Dumb way to disable Alt-Enter.
	{
		IDXGIFactory1 *factory = 0;
		if (swapchain->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<LPVOID*>(&factory)) == S_OK)
		{
			factory->MakeWindowAssociation (window_handle, DXGI_MWA_NO_ALT_ENTER);
			factory->Release();
		}
	}

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
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width    = static_cast<f32>(SCREEN_DIMENSIONS.x);
	viewport.Height   = static_cast<f32>(SCREEN_DIMENSIONS.y);
	device_context->RSSetViewports(1, &viewport);

	//
	// Shader.
	//

	ID3D10Blob* vertex_shader_blob;
	if (D3DCompileFromFile(SRC_DIR L"shaders/triangle.shader", 0, 0, "vertex_shader", "vs_4_0", 0, 0, &vertex_shader_blob, 0))
	{
		ASSERT(!"Failed to compile vertex shader.");
		return -1;
	}

	ID3D11VertexShader* vertex_shader;
	if (device->CreateVertexShader(vertex_shader_blob->GetBufferPointer(), vertex_shader_blob->GetBufferSize(), 0, &vertex_shader))
	{
		ASSERT(!"Failed to create vertex shader.");
		return -1;
	}
	DEFER { vertex_shader->Release(); };
	device_context->VSSetShader(vertex_shader, 0, 0);

	ID3D10Blob* pixel_shader_blob;
	if (D3DCompileFromFile(SRC_DIR L"shaders/triangle.shader", 0, 0, "pixel_shader", "ps_4_0", 0, 0, &pixel_shader_blob, 0))
	{
		ASSERT(!"Failed to compile pixel shader.");
		return -1;
	}

	ID3D11PixelShader* pixel_shader;
	if (device->CreatePixelShader(pixel_shader_blob->GetBufferPointer(), pixel_shader_blob->GetBufferSize(), 0, &pixel_shader))
	{
		ASSERT(!"Failed to create pixel shader.");
		return -1;
	}
	DEFER { pixel_shader->Release(); };
	device_context->PSSetShader(pixel_shader, 0, 0);

	struct Vertex
	{
		vf3 position;
		vf4 rgba;
	};

	constexpr Vertex TRIANGLE_VERTICES[] =
		{
			{ {  0.00f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
			{ {  0.45f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { -0.45f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
		};

	ID3D11Buffer* vertex_buffer;
	{
		D3D11_BUFFER_DESC buffer_description = {};
		buffer_description.Usage          = D3D11_USAGE_DYNAMIC;
		buffer_description.ByteWidth      = sizeof(TRIANGLE_VERTICES);
		buffer_description.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
		buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		device->CreateBuffer(&buffer_description, 0, &vertex_buffer);

		D3D11_MAPPED_SUBRESOURCE map;
		device_context->Map(vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
		memcpy(map.pData, TRIANGLE_VERTICES, sizeof(TRIANGLE_VERTICES));
		device_context->Unmap(vertex_buffer, 0);
	}
	DEFER { vertex_buffer->Release(); };

	D3D11_INPUT_ELEMENT_DESC input_element_descriptions[] =
		{
			{ "position", 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "rgba"    , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, rgba    ), D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

	ID3D11InputLayout* input_layout;
	if (device->CreateInputLayout(input_element_descriptions, ARRAY_CAPACITY(input_element_descriptions), vertex_shader_blob->GetBufferPointer(), vertex_shader_blob->GetBufferSize(), &input_layout))
	{
		ASSERT(!"Failed to create input layout.");
		return -1;
	}
	DEFER { input_layout->Release(); };

	//
	// Loop.
	//

	ShowWindow(window_handle, cmd_show);

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

			device_context->IASetInputLayout(input_layout);
			UINT stride = sizeof(Vertex);
			UINT offset = 0;
			device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
			device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			device_context->Draw(ARRAY_CAPACITY(TRIANGLE_VERTICES), 0);

			swapchain->Present(0, 0);
		}

		Sleep(1); // @TODO@ Cools the CPU. Should be better.
	}
	BREAK:;

	return 0;
}
