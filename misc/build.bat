@echo off
setlocal ENABLEDELAYEDEXPANSION

set DEBUG=1
set WARNINGS=-W4 -wd4201
set DEBUG_WARNINGS=%WARNINGS% -wd4505 -wd4100

IF NOT EXIST W:\build\ (
	mkdir W:\build\
)

pushd W:\build\
	if %DEBUG% equ 0 (
		echo Release build
	) else (
		echo Debug build
		cl /nologo /DDATA_DIR="\"W:/data/\"" /DEXE_DIR="\"W:/build/\"" /DSRC_DIR="\"W:/src/\"" /std:c++17 /Od /DDEBUG=1 /Z7 /MTd /GR- /EHsc /EHa- %DEBUG_WARNINGS% /WX /permissive- /Femetaprogram.exe W:\src\metaprogram.cpp /link /DEBUG:FULL /opt:ref /incremental:no

		if !ERRORLEVEL! neq 0 (
			goto metaprogram_failed
		)

		W:\build\metaprogram.exe

		if !ERRORLEVEL! neq 0 (
			goto metaprogram_failed
		)

		cl /nologo /DDATA_DIR="\"W:/data/\"" /DEXE_DIR="\"W:/build/\"" /DSRC_DIR="\"W:/src/\"" /std:c++17 /Od /DDEBUG=1 /Z7 /MTd /GR- /EHsc /EHa- %DEBUG_WARNINGS% /permissive- /FeMeat.exe W:\src\Meat.cpp /link /DEBUG:FULL /opt:ref /incremental:no
		goto end

		:metaprogram_failed
		echo Metaprogram failed.

		:end
		:
	)
popd

REM C4201       : "nonstandard extension used : nameless struct/union"
REM C4505       : "'function' : unreferenced local function has been removed"
REM /Z7         : "The /Z7 option produces object files that also contain full symbolic debugging information for use with the debugger."
REM /MTd        : "Defines _DEBUG and _MT. This option also causes the compiler to place the library name LIBCMTD.lib into the .obj file so that the linker will use LIBCMTD.lib to resolve external symbols."
REM /GR         : "Adds code to check object types at run time."
REM /EHsc       : "When used with /EHs, the compiler assumes that functions declared as extern "C" never throw a C++ exception."
REM /EHa        : "Enables standard C++ stack unwinding."
REM /permissive : "Specify standards conformance mode to the compiler."
