#include <stdint.h>
#include <stdio.h>


enum test_mode : uint32_t
{
    TestMode_Uninitialized,
    TestMode_Testing,
    TestMode_Completed,
    TestMode_Error,
};

struct repetition_test_results
{
    uint64_t TestCount;
    uint64_t TotalTime;
    uint64_t MaxTime;
    uint64_t MinTime;
};

struct repetition_tester
{
    uint64_t TargetProcessedByteCount;
    uint64_t CPUTimerFreq;
    uint64_t TryForTime;
    uint64_t TestsStartedAt;
    
    test_mode Mode;
    b32 PrintNewMinimums;
    uint32_t OpenBlockCount;
    uint32_t CloseBlockCount;
    uint64_t TimeAccumulatedOnThisTest;
    uint64_t BytesAccumulatedOnThisTest;

    repetition_test_results Results;
};

static double SecondsFromCPUTime(double CPUTime, uint64_t CPUTimerFreq)
{
    double Result = 0.0;
    if(CPUTimerFreq)
    {
        Result = (CPUTime / (double)CPUTimerFreq);
    }
    
    return Result;
}
 
static void PrintTime(char const *Label, double CPUTime, uint64_t CPUTimerFreq, 
                      uint64_t ByteCount)
{
    printf("%s: %.0f", Label, CPUTime);
    if(CPUTimerFreq)
    {
        double Seconds = SecondsFromCPUTime(CPUTime, CPUTimerFreq);
        printf(" (%fms)", 1000.0f*Seconds);
    
        if(ByteCount)
        {
            double Gigabyte = (1024.0f * 1024.0f * 1024.0f);
            double BestBandwidth = ByteCount / (Gigabyte * Seconds);
            printf(" %fgb/s", BestBandwidth);
        }
    }
}

static void PrintTime(char const *Label, uint64_t CPUTime, uint64_t CPUTimerFreq, 
                      uint64_t ByteCount)
{
    PrintTime(Label, (double)CPUTime, CPUTimerFreq, ByteCount);
}

static void PrintResults(repetition_test_results Results, uint64_t CPUTimerFreq, 
                         uint64_t ByteCount)
{
    PrintTime("Min", (double)Results.MinTime, CPUTimerFreq, ByteCount);
    printf("\n");
    
    PrintTime("Max", (double)Results.MaxTime, CPUTimerFreq, ByteCount);
    printf("\n");
    
    if(Results.TestCount)
    {
        PrintTime("Avg", (double)Results.TotalTime / (double)Results.TestCount, 
                  CPUTimerFreq, ByteCount);
        printf("\n");
    }
}

static void Error(repetition_tester *Tester, char const *Message)
{
    Tester->Mode = TestMode_Error;
    fprintf(stderr, "ERROR: %s\n", Message);
}

static void NewTestWave(repetition_tester *Tester, uint64_t TargetProcessedByteCount, 
                        uint64_t CPUTimerFreq, uint32_t SecondsToTry = 10)
{
    if(Tester->Mode == TestMode_Uninitialized)
    {
        Tester->Mode = TestMode_Testing;
        Tester->TargetProcessedByteCount = TargetProcessedByteCount;
        Tester->CPUTimerFreq = CPUTimerFreq;
        Tester->PrintNewMinimums = true;
        Tester->Results.MinTime = (uint64_t)-1;
    }
    else if(Tester->Mode == TestMode_Completed)
    {
        Tester->Mode = TestMode_Testing;
        
        if(Tester->TargetProcessedByteCount != TargetProcessedByteCount)
        {
            Error(Tester, "TargetProcessedByteCount changed");
        }
        
        if(Tester->CPUTimerFreq != CPUTimerFreq)
        {
            Error(Tester, "CPU frequency changed");
        }
    }

    Tester->TryForTime = SecondsToTry*CPUTimerFreq;
    Tester->TestsStartedAt = ReadCPUTimer();
}

static void BeginTime(repetition_tester *Tester)
{
    ++Tester->OpenBlockCount;
    Tester->TimeAccumulatedOnThisTest -= ReadCPUTimer();
}

static void EndTime(repetition_tester *Tester)
{
    ++Tester->CloseBlockCount;
    Tester->TimeAccumulatedOnThisTest += ReadCPUTimer();
}

static void CountBytes(repetition_tester *Tester, uint64_t ByteCount)
{
    Tester->BytesAccumulatedOnThisTest += ByteCount;
}


static b32 IsTesting(repetition_tester *Tester)
{
    if(Tester->Mode == TestMode_Testing)
    {
        uint64_t CurrentTime = ReadCPUTimer();
        
        if(Tester->OpenBlockCount)
        {
            if(Tester->OpenBlockCount != Tester->CloseBlockCount)
            {
                Error(Tester, "Unbalanced BeginTime/EndTime");
            }
            
            if(Tester->BytesAccumulatedOnThisTest != Tester->TargetProcessedByteCount)
            {
                Error(Tester, "Processed byte count mismatch");
            }
    
            if(Tester->Mode == TestMode_Testing)
            {
                repetition_test_results *Results = &Tester->Results;
                uint64_t ElapsedTime = Tester->TimeAccumulatedOnThisTest;
                Results->TestCount += 1;
                Results->TotalTime += ElapsedTime;
                if(Results->MaxTime < ElapsedTime)
                {
                    Results->MaxTime = ElapsedTime;
                }
                
                if(Results->MinTime > ElapsedTime)
                {
                    Results->MinTime = ElapsedTime;

                    Tester->TestsStartedAt = CurrentTime;
                    
                    if(Tester->PrintNewMinimums)
                    {
                        PrintTime("Min", Results->MinTime, Tester->CPUTimerFreq, 
                                  Tester->BytesAccumulatedOnThisTest);
                        printf("               \r");
                    }
                }
                
                Tester->OpenBlockCount = 0;
                Tester->CloseBlockCount = 0;
                Tester->TimeAccumulatedOnThisTest = 0;
                Tester->BytesAccumulatedOnThisTest = 0;
            }
        }
        
        if((CurrentTime - Tester->TestsStartedAt) > Tester->TryForTime)
        {
            Tester->Mode = TestMode_Completed;
            
            printf("                                                          \r");
            PrintResults(Tester->Results, Tester->CPUTimerFreq, 
                         Tester->TargetProcessedByteCount);
        }
    }
    
    b32 Result = (Tester->Mode == TestMode_Testing);
    return Result;
}
