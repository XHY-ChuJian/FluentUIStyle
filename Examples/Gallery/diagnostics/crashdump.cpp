#include "crashdump.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dbghelp.h>

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cwchar>
#include <exception>

namespace
{
constexpr DWORD TerminateExceptionCode = 0xE0000001;
constexpr DWORD PureCallExceptionCode = 0xE0000002;
constexpr DWORD InvalidParameterExceptionCode = 0xE0000003;
constexpr DWORD AbortExceptionCode = 0xE0000004;
constexpr int MaximumPathLength = 32768;

wchar_t g_dumpDirectory[MaximumPathLength]{};

LONG writeDump( EXCEPTION_POINTERS* exceptionPointers )
{
    if ( g_dumpDirectory[0] == L'\0' )
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    SYSTEMTIME time{};
    GetLocalTime( &time );

    const DWORD processId = GetCurrentProcessId();
    const DWORD exceptionCode = exceptionPointers && exceptionPointers->ExceptionRecord
        ? exceptionPointers->ExceptionRecord->ExceptionCode
        : 0;
    wchar_t dumpPath[MaximumPathLength]{};
    const int pathLength = swprintf_s(
        dumpPath,
        MaximumPathLength,
        L"%ls\\Gallery_%04u%02u%02u_%02u%02u%02u_%03u_pid%lu_%08lX.dmp",
        g_dumpDirectory,
        static_cast<unsigned int>( time.wYear ),
        static_cast<unsigned int>( time.wMonth ),
        static_cast<unsigned int>( time.wDay ),
        static_cast<unsigned int>( time.wHour ),
        static_cast<unsigned int>( time.wMinute ),
        static_cast<unsigned int>( time.wSecond ),
        static_cast<unsigned int>( time.wMilliseconds ),
        processId,
        exceptionCode );
    if ( pathLength <= 0 )
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    const HANDLE dumpFile = CreateFileW( dumpPath,
                                         GENERIC_WRITE,
                                         FILE_SHARE_READ,
                                         nullptr,
                                         CREATE_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL,
                                         nullptr );
    if ( dumpFile == INVALID_HANDLE_VALUE )
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    MINIDUMP_EXCEPTION_INFORMATION exceptionInformation{};
    exceptionInformation.ThreadId = GetCurrentThreadId();
    exceptionInformation.ExceptionPointers = exceptionPointers;
    exceptionInformation.ClientPointers = FALSE;

    const MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
        MiniDumpWithDataSegs |
        MiniDumpWithHandleData |
        MiniDumpWithUnloadedModules |
        MiniDumpWithThreadInfo );
    MiniDumpWriteDump( GetCurrentProcess(),
                       processId,
                       dumpFile,
                       dumpType,
                       exceptionPointers ? &exceptionInformation : nullptr,
                       nullptr,
                       nullptr );
    CloseHandle( dumpFile );
    return EXCEPTION_EXECUTE_HANDLER;
}

LONG WINAPI unhandledExceptionFilter( EXCEPTION_POINTERS* exceptionPointers )
{
    return writeDump( exceptionPointers );
}

[[noreturn]] void raiseFatalException( DWORD exceptionCode )
{
    RaiseException( exceptionCode, EXCEPTION_NONCONTINUABLE, 0, nullptr );
    TerminateProcess( GetCurrentProcess(), exceptionCode );
    std::abort();
}

[[noreturn]] void terminateHandler()
{
    raiseFatalException( TerminateExceptionCode );
}

void pureCallHandler()
{
    raiseFatalException( PureCallExceptionCode );
}

void invalidParameterHandler( const wchar_t*,
                              const wchar_t*,
                              const wchar_t*,
                              unsigned int,
                              uintptr_t )
{
    raiseFatalException( InvalidParameterExceptionCode );
}

void abortSignalHandler( int )
{
    raiseFatalException( AbortExceptionCode );
}
}
#endif

QString GalleryCrashDump::install()
{
#ifdef Q_OS_WIN
    QString dumpDirectory = QDir( QCoreApplication::applicationDirPath() )
                                .filePath( QStringLiteral( "crashdumps" ) );
    if ( !QDir().mkpath( dumpDirectory ) )
    {
        dumpDirectory = QDir( QStandardPaths::writableLocation(
                                  QStandardPaths::AppLocalDataLocation ) )
                            .filePath( QStringLiteral( "crashdumps" ) );
        if ( !QDir().mkpath( dumpDirectory ) )
        {
            return {};
        }
    }

    const QString nativeDirectory = QDir::toNativeSeparators(
        QDir( dumpDirectory ).absolutePath() );
    // Leave enough room for the timestamp, PID and exception-code suffix so
    // the crash path can be formatted without invoking the CRT parameter handler.
    if ( nativeDirectory.size() >= MaximumPathLength - 128 )
    {
        return {};
    }
    const int copiedLength = nativeDirectory.toWCharArray( g_dumpDirectory );
    g_dumpDirectory[copiedLength] = L'\0';

    SetUnhandledExceptionFilter( unhandledExceptionFilter );
    std::set_terminate( terminateHandler );
    _set_purecall_handler( pureCallHandler );
    _set_invalid_parameter_handler( invalidParameterHandler );
    std::signal( SIGABRT, abortSignalHandler );
    return QDir::fromNativeSeparators( nativeDirectory );
#else
    return {};
#endif
}
