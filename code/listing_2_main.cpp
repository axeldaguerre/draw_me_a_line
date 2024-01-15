#include<windows.h>
#include"math.h"
#include<stdio.h>
#include<stdint.h>

typedef int32_t b32;

static int32_t LineColor  = (255 << 16) | (0 << 8) | (0 << 0);
static int32_t DebugColor  = (0 << 16) | (255 << 8) | (0 << 0);

struct win32_window_dimension
{
    int Width;
    int Height;
};

enum draw_line_method
{
    Line_Draw_By_Rounding,
    Line_Draw_By_Bresenham,
    
    Line_Draw_Count,
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

inline int32_t RoundReal32Toint32_t(float Real32)
{
    int32_t Result = (int32_t)roundf(Real32); /* Round to the nearest integer */
    return(Result);
}

inline uint32_t RoundReal32ToUint32_t(float Real32)
{
    uint32_t Result = (uint32_t)roundf(Real32); /* Round to the nearest integer unsigned */
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
    /*
        NOTE(Axel): When wanting to replicate a line on a rasterized screen, a problem arise: 
          your program try to real points on the line but it can place the points only 
          on integer position instead (finite-resolution). A good program handling it 
          will compute it fast enough, with almost unpersceptible inaccuracy. 

          Keep in mind that generaly if not always a screen coordinate will flow 
          from top left to bottom right, so it's flipped from the coordinate we 
          see in mathematics.

          All methods will use the formula [y = mx + b]'s derivation.
          As we know two points of the line, we don't need to know the y intercept, we
          can derive the next point from one of the point and make the decision of wich
          pixel we will draw, knowing the major dimension (x for -1 < m < 1, y for the other) 
          The main aspect of most of the algorithm to draw lines on rasterized screens is
          that they are incremental: they use previous calculation in order to compute the
          next pixel to draw. For example you can avoid calculating the y intercept
          as the previous (x0,y0) will have it and will never change from there.
          Good ressources for the subject are :
                - "Computer Graphics: Principles and Practice (Addison-Wesley, 1990)" p73
                - "Michael Abrash's Graphics programming black book" p660 
    */     
    switch(Method)
    {
        case Line_Draw_By_Rounding:
        {
        /*
            NOTE(Axel): Draw Pixel for -1f < m < 1f __With floating point__.
              The algorithm make the decision of going +1 in minor or +0 by rounding the 
              majorDimension+m and by keeping track of the m of the real y 
              __being a fractional__. 
              I didn't cover all the m spectrum because this algorithm is not the way 
              you would do it. It's just here for educational purpose and profile it 
              when the profiler is created.
        */
        int32_t dx          = (X1 - X0);
        int32_t dy          = (Y1 - Y0);
        float m            = (float)dy / (float)dx;
        float YOnTheLine   = (float)Y0;
        int32_t Y           = 0;

        if(m < 1.0f || m > -1.0f)
        {
          for(int32_t X = X0; X < X1; ++X)
          {
            YOnTheLine += m;
            Y = RoundReal32Toint32_t(YOnTheLine);

            DrawPixel(Buffer, X, Y, Color);
          }  
        }
        } break;

        case Line_Draw_By_Bresenham:
        {
          /*
            NOTE(Axel): Bresenham algorithm use the value of f(x,y+1/2) in order to 
              make a decision on wich pixel to choose. 
              Because everything (line and pixel grid) is constant, we are able to 
              derive constants from the line and the pixel grid.
              Derivation of f(x,y) = mx + B gives:
              0 = a.x + b.y + c
              where a = dy, b = -dx and c = dx.B
          */
         /*
            We store the value of the decision d increment from for x0+1 and store it 
            as a constant doing it for PixelE and PixelNE. Depending on the pixel choosen we 
            will add the value to the decision variable d. 
            In order to find the constant increment, as always we use algebra and 
            replace the derivation, calculate the decision for {x0,y0} on East and the next
            mid point ({x0+2,y0+0.5). We do the same for the NE being a little bit tricker
            as we have incremented the y by one compared to the Pixel East. Then:
            Pixel East:
              dOld = (a(x+1) + b(y+0.5) + c)
              dNew = (a(x+2) + b(y+0.5) + c)
              decisionE  = (dOld - dNew) = a = dy
            Pixel Nort East:
              dOld = (a(x+1) + b(y+1.5) + c)
              dOld = (a(x+2) + b(y+2.5) + c)
              decisionNE  = (dOld - dNew) = a + b = dy + (-dx)
         */
          /*
            Every decision values are multiply by two in order to avoid floating point 
            without changing the sign of the decision value though our computation's truth
          */ 
          int32_t dx          = (X1 - X0);
          int32_t dy          = (Y1 - Y0);
          int32_t decision    = (2 * dy) - dx;   // 0 + dy - (0.5 * dx) 
          int32_t IncrementNE = (2 * (dy - dx)); // dy - dx
          int32_t IncrementE  = (2 * dy);        // dy
          int32_t Y           = Y0;

          for(int32_t X = X0; X < (X0+dx); ++X)
          {
            if(decision <= 0) 
            {
              decision += IncrementE; 
            }
            else 
            {
              ++Y;
              decision += IncrementNE;
            }

            DrawPixel(Buffer, X, Y, Color); 
          }

        } break;

        default:
        {
            printf("Line drawing algorithm not implemented yet\n");
        }
    }   
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

int CALLBACK WinMain(HINSTANCE Instance,
                    HINSTANCE PrevInstance,
                    LPSTR CommandLine,
                    int ShowCode)
{
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1920, 1080);
    
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

                for(;;)
                {
                    for(uint32_t LineDrawIndex = 0; 
                        LineDrawIndex < Line_Draw_Count; 
                        ++LineDrawIndex)
                    {
                        draw_line_method Method = (draw_line_method)LineDrawIndex;
                        
                        DrawLine(&Buffer, X0, Y0, X1, Y1, LineColor, Line_Draw_By_Bresenham);
                        
                        Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext);                      
                    }
                }                
            }
        }                                
    }
      
    return(0);
}
