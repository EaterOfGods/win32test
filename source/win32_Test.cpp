/*  ===================================================================
	File: win32_Test.cpp
	Date: 10 January 2017 (Tuesday)
	Revision: 11 January 2017 (Wednesday)
	
	Author: Tony Persson
	Contact: Nepra_84@msn.com
	===================================================================  */

#include <windows.h>
#include <stdint.h>

#define global_variable static
#define local_persist static
#define internal static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32; 
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct win32_offscreen_buffer
{
	// NOTE(Tony): Pixels are always 32-bits wide, Memory order BB GG RR XX
	BITMAPINFO Info;
	void *Memory;
	int Width;
 	int Height;
 	int Pitch;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

// TODO(Tony): don't use globals.
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;

win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}

internal void
RenderWeirdGradient(win32_offscreen_buffer Buffer, int BlueOffset, int GreenOffset)
{
	uint8 *Row = (uint8 *)Buffer.Memory;
	for(int Y = 0;
		Y < Buffer.Height; 
		++Y)
	{
		uint32 *Pixel = (uint32 *)Row;

		for(int X = 0; 
			X < Buffer.Width; 
			++X)
		{
			uint8 Blue = (X +  BlueOffset); 
			uint8 Green = (Y + GreenOffset);							

			*Pixel++ = ((Green << 8) | Blue);
		}

		Row += Buffer.Pitch;
	}
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	if(Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	// NOTE(Tony): Using padding for memory alignment.
	int BytesPerPixel = 4;

	// NOTE(Tony): When the biHeight field is negative, this is the clue to
	// Windows to treat this bitmap as top-down, not bottom-up, meaning that
	// the first three bytes of the image are the colors of the top left pixel
	// in the bitmap, not the bottom left!
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height; 
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Width*Height)*BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	
	Buffer->Pitch = Width*BytesPerPixel;
	
	// TODO(Tony): Clear this to black?

} 

internal void
Win32DisplayBufferInWindow(HDC DeviceContext, 
						   int WindowWidth, int WindowHeight, 
						   win32_offscreen_buffer Buffer,
						   int X, int Y)
{
	StretchDIBits(DeviceContext,
				  /*
				  X, Y, Width, Height,
				  X, Y, Width, Height,
				  */
				  0, 0, WindowWidth, WindowHeight,
				  0, 0, Buffer.Width, Buffer.Height,
				  Buffer.Memory,
				  &Buffer.Info,
				  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
				   UINT Message,
				   WPARAM WParam,
				   LPARAM LParam)
{
	LRESULT Result = 0; 

	switch(Message)
	{
		case WM_SIZE:
		{
		} break;

		case WM_DESTROY:
		{
			// TODO(Tony): Handle this as an error, recreate window.
			Running = false;
		} break;

		case WM_CLOSE:
		{
			// TODO(Tony): Ask the user if they want to quit.
			Running = false;
		} break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

			// TODO(Tony): fix this shit.	
			win32_window_dimension Dimension = Win32GetWindowDimension(Window);

			Win32DisplayBufferInWindow(DeviceContext,
									   Dimension.Width, Dimension.Height,
									   GlobalBackbuffer,
									   X, Y);
			EndPaint(Window, &Paint);
		} break;

		default:
		{
			Result = DefWindowProc(Window, Message, WParam, LParam);
		} break;

	}

	return Result;
}

int CALLBACK 
WinMain(HINSTANCE Instance,
		HINSTANCE PrevInstance,
		LPSTR CommandLine,
		int ShowCode)
{
	
	WNDCLASS WindowClass = {};

	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);
	
	WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "TestWindowClass";
	//WindowClass.hIcon =;

	if(RegisterClass(&WindowClass))
	{
		HWND Window = 
					CreateWindowEx(	
						0,
  						WindowClass.lpszClassName,
  						"Win32 Test",
  						WS_OVERLAPPEDWINDOW|WS_VISIBLE,
  						CW_USEDEFAULT,
  						CW_USEDEFAULT,
  						CW_USEDEFAULT,
  						CW_USEDEFAULT,
  						0,
  						0,
  						Instance,
  						0);

		if(Window)
		{
			HDC DeviceContext = GetDC(Window);

			int XOffset = 0;
			int YOffset = 0;

			Running = true;
			while(Running)
			{
				MSG Message;
				while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if(Message.message == WM_QUIT)
					{
						Running = false;
					}

					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(DeviceContext,
										   Dimension.Width, Dimension.Height,
										   GlobalBackbuffer,
										   0, 0);
				++XOffset;
				YOffset += 2;

			}
		}
		else
		{
			// TODO(Tony): Logging
		}

	}
	else
	{
		// TODO(Tony): Logging
	}
	

	return 0;
}
