#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <stdbool.h>
#include <errno.h>

HANDLE canRead;
HANDLE canWrite;
HANDLE mutex;

LONG activeReaders = 0;
LONG activeWriter = FALSE;
LONG writersCount = 0;
LONG readersCount = 0;

INT cnt = 0;
CHAR letter = 'a';

BOOL flag = 1;
BOOL sigHandler(DWORD signal)
{
    flag = 0;
    return TRUE;
}

void startRead()
{
    InterlockedIncrement(&readersCount);
    if (writersCount || WaitForSingleObject(canWrite, 0) == WAIT_OBJECT_0)
    {
        if (WaitForSingleObject(canRead, INFINITE) != WAIT_OBJECT_0)
        {
            printf("errno: %d", errno);
            perror("WaitForSingleObject");
            exit(1);
        }
    }
    if (WaitForSingleObject(mutex, INFINITE) == WAIT_FAILED)
    {
        printf("errno: %d", errno);
        perror("WaitForSingleObject");
        exit(1);
    }
    if (!SetEvent(canRead))
    {
        printf("errno: %d", errno);
        perror("SetEvent");
        exit(1);
    }
    InterlockedDecrement(&readersCount);
    InterlockedIncrement(&activeReaders);
    if (!ReleaseMutex(mutex))
    {
        printf("errno: %d", errno);
        perror("ReleaseMutex");
        exit(1);
    }
}

void stopRead()
{
    InterlockedDecrement(&activeReaders);
    if (activeReaders == 0)
    {
        if (!SetEvent(canWrite))
        {
            printf("errno: %d", errno);
            perror("SetEvent");
            exit(1);
        }
    }
}

void startWrite()
{
    InterlockedIncrement(&writersCount);
    if (activeWriter || WaitForSingleObject(canRead, 0) == WAIT_OBJECT_0)
    {
        if (WaitForSingleObject(canWrite, INFINITE) == WAIT_FAILED)
        {
            printf("errno: %d", errno);
            perror("WaitForSingleObject");
            exit(1);
        }
    }
    if (WaitForSingleObject(mutex, INFINITE) == WAIT_FAILED)
    {
        printf("errno: %d", errno);
        perror("WaitForSingleObject");
        exit(1);
    }
    InterlockedDecrement(&writersCount);
    InterlockedExchange(&activeWriter, TRUE);
    if (!ReleaseMutex(mutex))
    {
        printf("errno: %d", errno);
        perror("ReleaseMutex");
        exit(1);
    }
}

void stopWrite()
{
    if (!ResetEvent(canWrite))
    {
        printf("errno: %d", errno);
        perror("ResetEvent");
        exit(1);
    }
    InterlockedExchange(&activeWriter, FALSE);
    if (readersCount > 0)
    {
        if (!SetEvent(canRead))
        {
            printf("errno: %d", errno);
            perror("SetEvent");
            exit(1);
        }
    }
    else
    {
        if (!SetEvent(canWrite))
        {
            printf("errno: %d", errno);
            perror("SetEvent");
            exit(1);
        }
    }
}

DWORD Reader(PVOID param)
{
    while (flag)
    {
        startRead();
        printf("Reader %ld = %c\n", GetCurrentThreadId(), letter);
        stopRead();
    }
    return 0;
}

DWORD Writer(PVOID param)
{
    while (flag)
    {
        startWrite();
        if (letter == 'z')
        {
            cnt++;
            letter = 'a';
            if (cnt == 3)
            {
                if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0))
                {
                    printf("errno: %d", errno);
                    perror("GenerateConsoleCtrlEvent");
                    exit(1);
                }
            }
        }
        else
            letter++;
        printf("Writer %ld = %c\n", GetCurrentThreadId(), letter);
        stopWrite();
    }
    return 0;
}

int main(void)
{
    setbuf(stdout, NULL);

    if (!SetConsoleCtrlHandler(sigHandler, TRUE))
    {
        printf("errno: %d", errno);
        perror("SetConsoleCtrlHandler");
        exit(1);
    }

    const int writerAmount = 5;
    const int readerAmount = 3;
    DWORD thid[writerAmount + readerAmount];
    HANDLE pthread[writerAmount + readerAmount];

    if ((canRead = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
    {
        printf("errno: %d", errno);
        perror("CreateEvent");
        exit(1);
    }
    if ((canWrite = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
    {
        printf("errno: %d", errno);
        perror("CreateEvent");
        exit(1);
    }
    if ((mutex = CreateMutex(NULL, 0, NULL)) == NULL)
    {
        printf("errno: %d", errno);
        perror("CreateMutex");
        exit(1);
    }

    for (int i = 0; i < writerAmount; i++)
    {
        pthread[i] = CreateThread(NULL, 0, Writer, NULL, 0, &thid[i]);
        if (pthread[i] == NULL)
        {
            printf("errno: %d", errno);
            perror("CreateThread");
            exit(1);
        }
    }
    for (int i = writerAmount; i < writerAmount + readerAmount; i++)
    {
        pthread[i] = CreateThread(NULL, 0, Reader, NULL, 0, &thid[i]);
        if (pthread[i] == NULL)
        {
            printf("errno: %d", errno);
            perror("CreateThread");
            exit(1);
        }
    }

    for (int i = 0; i < writerAmount + readerAmount; i++)
    {
        DWORD dw = WaitForSingleObject(pthread[i], 5000);
        switch (dw)
        {
        case WAIT_OBJECT_0:
            printf("thread %ld finished\n", thid[i]);
            break;
        case WAIT_TIMEOUT:
            printf("waitThread timeout %ld\n", dw);
            break;
        case WAIT_FAILED:
            printf("waitThread failed %ld\n", dw);
            break;
        default:
            printf("unknown %ld\n", dw);
            break;
        }
    }

    CloseHandle(canRead);
    CloseHandle(canWrite);
    CloseHandle(mutex);
    return 0;
}
