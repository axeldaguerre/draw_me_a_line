#include<windows.h>
#include"math.h"
#include<stdio.h>
#include<stdint.h>

typedef int32_t b32;
typedef float real32;
typedef double real64;

static int32_t LineColor  = (255 << 16) | (0 << 8) | (0 << 0);
static int32_t DebugColor  = (0 << 16) | (255 << 8) | (0 << 0);

enum draw_line_method
{
    Line_Draw_By_Rounding,
    Line_Draw_By_Pythagore,
    Line_Draw_By_Bresenham,
    
    Line_Draw_Count,
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
    int BitmapMemorySize;
};

static b32 GlobalRunning;
static win32_offscreen_buffer GlobalBackBuffer;

inline int32_t RoundReal32Toint32_t(real32 Real32)
{
    int32_t Result = (int32_t)roundf(Real32); /* Round to the nearest integer */
    return(Result);
}

static void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    int BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    Buffer->BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, Buffer->BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, 
                                  PAGE_READWRITE);
    Buffer->Pitch = Width * BytesPerPixel;
}

static void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                           HDC DeviceContext)
{
    StretchDIBits(DeviceContext,
                    0, 0, Buffer->Width, Buffer->Height,
                    0, 0, Buffer->Width, Buffer->Height,
                    Buffer->Memory,
                    &Buffer->Info,
                    DIB_RGB_COLORS, SRCCOPY);

}

static win32_window_dimension Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}

static LRESULT CALLBACK Win32MainWindowCallback(HWND Window,
                                                UINT Message,
                                                WPARAM WParam,
                                                LPARAM LParam)
{       
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_CLOSE:
        {
            GlobalRunning = false;
        } break;

        case WM_SETCURSOR:
        {

        } break;
        
        case WM_ACTIVATEAPP:
        {

        } break;

        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return(Result);    
}


void DrawPixel(win32_offscreen_buffer *Buffer, 
              int32_t X0, int32_t Y0, int32_t Color)
{    
    int32_t Y = Y0 * Buffer->Pitch;
    int32_t X = X0 * Buffer->BytesPerPixel;
    uint8_t *Row = (uint8_t*)Buffer->Memory + X + Y;
    
    int32_t *Pixel = (int32_t*)Row;
    *Pixel = Color;
}

inline void DrawLine(win32_offscreen_buffer *Buffer, 
              int32_t X0, int32_t Y0, 
              int32_t X1, int32_t Y1, int32_t Color, 
              draw_line_method Method)
{ 
    switch(Method)
    {
        case Line_Draw_By_Rounding:
        {
            /* TODO: By using m with y = mx + b 
                    and rouding with RoundReal32Toint32_t function,
                    you will be able to find the nearest Y coordinate of 
                    the pixel.
            */
        } break;

        case Line_Draw_By_Bresenham:
        {
        /*
            TODO: With the note from the previous course, you can try to implement the 
                Beresenham algorithm. Only 1 octant is asked here.
            Use :
                - https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
                - If you can have access to Computer Graphics Principles - Andries van Dam p90
        */
        } break;

        default:
        {
            printf("Line drawing algorithm not implemented yet\n");
        }
    }   
}

int CALLBACK WinMain(HINSTANCE Instance,
                    HINSTANCE PrevInstance,
                    LPSTR CommandLine,
                    int ShowCode)
{
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 960, 540);
    
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.lpszClassName = "LineDrawing";

    if(RegisterClassA(&WindowClass))
    {
        GlobalRunning = true;
        
        HWND Window =
            CreateWindowExA(
                0,
                WindowClass.lpszClassName,
                "Draw me a line",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                GlobalBackBuffer.Width,
                GlobalBackBuffer.Height,
                0,
                0,
                Instance,
                0
            );

        
        if(Window)
        {   
            int32_t X0 = 10;
            int32_t Y0 = 10;
            int32_t X1 = 200;
            int32_t Y1 = 80;

            while(GlobalRunning)
            {                    
                win32_offscreen_buffer Buffer = {};
                Buffer.Memory = GlobalBackBuffer.Memory;
                Buffer.Width = GlobalBackBuffer.Width; 
                Buffer.Height = GlobalBackBuffer.Height;
                Buffer.Pitch = GlobalBackBuffer.Pitch;
                Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;
                
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                HDC DeviceContext = GetDC(Window);

                DrawLine(&Buffer, X0, Y0, X1, Y1, LineColor, Line_Draw_By_Bresenham);
                Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext);

            }
        }                                
    }
      
    return(0);
}

