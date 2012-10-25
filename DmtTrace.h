#ifndef _DMT_TRACE
#define _DMT_TRACE

#include <stdio.h>
#include <windows.h>

namespace DM
{
#ifdef _DEBUG

	class DebugTraceConsole
	{
	private:
		static DebugTraceConsole *dtc;

		DebugTraceConsole(const char *console_title)
		{
			::FreeConsole();
			::AllocConsole();
			::SetConsoleTitleA(console_title);
		}

	public:
		static void Create(const char *console_title)
		{
			if (!dtc) dtc = new DebugTraceConsole(console_title);
		}
	};

	inline void _trace(char *format, ...) 
	{ 
		static char buffer[2048] = {0};
		DWORD rw = 0;

		DebugTraceConsole::Create((const char *)"DMT debug trace");
		
		va_list argptr; 
		va_start(argptr, format); 
		vsprintf_s(buffer, format, argptr); 
		va_end(argptr);

		::WriteConsoleA(::GetStdHandle(STD_OUTPUT_HANDLE), buffer, strlen(buffer), &rw, NULL);
	}

#define DMT_TRACE _trace 

#else 

#define DMT_TRACE					__noop

#endif
}

#endif