/*  ===================================================================
	File: win32_Test.cpp
	Date: 10 January 2017 (Tuesday)
	Revision: 
	
	Author: Tony Persson
	Contact: Nepra_84@msn.com
	Notice: (C) Copyright 2017 by Tony Persson. All Rights Reserved.
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

// TODO(Tony): don't use globals.
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void
RenderWeirdGradient(int XOffset, int YOffset)
{
	int Width = BitmapWidth;
	int Height = BitmapHeight;

	int Pitch = Width*BytesPerPixel;
	uint8 *Row = (uint8 *)BitmapMemory;
	for(int Y = 0;
		Y < BitmapHeight; 
		++Y)
	{
		uint8 *Pixel = (uint8 *)Row;

		for(int X = 0; 
			X < BitmapWidth; 
			++X)
		{
			*Pixel = (uint8)(X + XOffset); 	//B
			++Pixel;

			*Pixel = (uint8)(Y + YOffset); 	//G
			++Pixel;

			*Pixel = 0;						//R		
			++Pixel;

			*Pixel = 0;						//A
			++Pixel;
		}

		Row += Pitch;
	}
}

internal void
Win32ResizeDIBSection(int Width, int Height)
{
	if(BitmapMemory)
	{
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapWidth = Width;
	BitmapHeight = Height;

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight; // NOTE(Tony): Negative for top->down.
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	// NOTE(Tony): Using padding for memory alignment.

	int BitmapMemorySize = (Width*Height)*BytesPerPixel;
	BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	RenderWeirdGradient(0, 0);

	// TODO(Tony): Clear this to black.
} 

internal void
Win32UpdateWindow(HDC DeviceContext, RECT *WindowRect, int X, int Y, int Width, int Height)
{
	int WindowWidth = WindowRect->right - WindowRect->left;
	int WindowHeight = WindowRect->bottom - WindowRect->top;

	StretchDIBits(DeviceContext,
				  /*
				  X, Y, Width, Height,
				  X, Y, Width, Height,
				  */
				  0, 0, BitmapWidth, BitmapHeight,
				  0, 0, WindowWidth, WindowHeight,
				  BitmapMemory,
				  &BitmapInfo,
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
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			Win32ResizeDIBSection(Width, Height);
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
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);

			Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
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
	
	WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "TestWindowClass";
	//WindowClass.hIcon =;

	if(RegisterClass(&WindowClass))
	{
		HWND WindowHandle = 
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

		if(WindowHandle)
		{
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
