#include<stdint.h>
#include<math.h>
#include<windows.h>
#include<stdio.h>

typedef int32_t b32;
typedef float real32;
typedef double real64;

static int32_t LineColor  = (255 << 16) | (0 << 8) | (0 << 0);
static int32_t DebugColor  = (0 << 16) | (0 << 8) | (255 << 0);

#include "shared_platform_metrics.cpp"
#include "shared_repetition_tester.cpp"

struct screen_buffer
{
  uint32_t Width;
  uint32_t Height;
  uint32_t BytesPerPixel;
  uint32_t Pitch;
  
  uint8_t *Memory;
  size_t MemoryCount;
};

static screen_buffer GlobalBuffer;

enum draw_line_method
{
    Line_Draw_By_Rounding,
    Line_Draw_By_Bresenham,
    
    Line_Draw_Count,
};

struct buffer
{
    size_t Count;
    uint8_t *Data;
};

static buffer AllocateBuffer(size_t Count)
{
    buffer Result = {};
    Result.Data = (uint8_t *)malloc(Count);
    if(Result.Data)
    {
        Result.Count = Count;
    }
    else
    {
        fprintf(stderr, "ERROR: Unable to allocate %llu bytes.\n", Count);
    }
    
    return Result;
}

inline int32_t RoundReal32Toint32_t(real32 Real32)
{
    int32_t Result = (int32_t)roundf(Real32); /* Round to the nearest integer */
    return(Result);
}

b32 InitScreenBuffer()
{
  b32 Result = false;

  GlobalBuffer.Width = 1920;
  GlobalBuffer.Height = 1080;
  GlobalBuffer.BytesPerPixel = 4;
  GlobalBuffer.Pitch = GlobalBuffer.Width*GlobalBuffer.BytesPerPixel;

  buffer Buffer = AllocateBuffer(GlobalBuffer.Height*GlobalBuffer.Width*GlobalBuffer.BytesPerPixel);
  GlobalBuffer.Memory = Buffer.Data;
  GlobalBuffer.MemoryCount = Buffer.Count;

  if(GlobalBuffer.Memory)
  {
    Result = true; 
  }

  return Result;
}  

void DrawPixel(screen_buffer *Buffer, 
              int32_t X0, int32_t Y0, int32_t Color)
{    
    int32_t Y = Y0 * Buffer->Pitch;
    int32_t X = X0 * Buffer->BytesPerPixel;
    uint8_t *Row = (uint8_t*)Buffer->Memory + X + Y;
    
    int32_t *Pixel = (int32_t*)Row;
    *Pixel = Color;
}

inline void DrawLine(screen_buffer *Buffer, 
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
          int32_t decision    = (2 * dy) - dx; // 0 + dy - (0.5 * dx)
          int32_t IncrementNE = (2 * (dy - dx)); // dy - dx
          int32_t IncrementE  = (2 * dy);      // dy
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

            DrawPixel(Buffer, X, Y, LineColor); 
          }

        } break;

        default:
        {
            printf("Line drawing algorithm not implemented yet\n");
        }
    }   
}

inline void PrintLineDrawingMethod(draw_line_method Method)
{
    printf("======= Line Drawing: ");

    switch(Method) 
    {       
        case Line_Draw_By_Rounding:
        {
            printf("By Rounding");
        } break;

        case Line_Draw_By_Bresenham:
        {
            printf("With Bresenham");
        } break;

        default:
        {
            printf("Not implemented");
        }
    }

    printf("======= \n");
}

int main()
{
    repetition_tester Testers[Line_Draw_Count] = {};
    uint64_t CPUTimerFreq = EstimateCPUTimerFreq();            

    if(InitScreenBuffer())
    {
        int32_t X0 = 10;
        int32_t Y0 = 10;
        int32_t X1 = 200;
        int32_t Y1 = 80;

        for(;;)
        {
            for(uint32_t LineDrawIndex = 0; 
                LineDrawIndex < Line_Draw_Count; 
                ++LineDrawIndex)
            {
                draw_line_method Method = (draw_line_method)LineDrawIndex;
                PrintLineDrawingMethod(Method);

                repetition_tester *Tester = &Testers[LineDrawIndex];
                NewTestWave(Tester, GlobalBuffer.MemoryCount, CPUTimerFreq);
            
                while(IsTesting(Tester))
                {
                    BeginTime(Tester);
                    DrawLine(&GlobalBuffer, X0, Y0, X1, Y1, LineColor, Method);
                    EndTime(Tester);
                    
                    CountBytes(Tester, GlobalBuffer.MemoryCount);
                }
            }
        }
    } 
    else 
    {
      printf("ERROR: Could not allocate memory for buffer \n");
    }
      
    return(0);
}