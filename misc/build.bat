@echo off
setlocal ENABLEDELAYEDEXPANSION

set DEBUG=1
set WARNINGS=-W4 -wd4201
set DEBUG_WARNINGS=%WARNINGS% -wd4505 -wd4100 -wd4101
set LIBRARIES=user32.lib d3d11.lib d3dcompiler.lib

IF NOT EXIST W:\build\ (
	mkdir W:\build\
)

pushd W:\build\
	if %DEBUG% equ 0 (
		echo Release build
	) else (
REM		echo Debug build
REM		cl /nologo /DDATA_DIR="\"W:/data/\"" /DEXE_DIR="\"W:/build/\"" /DSRC_DIR="\"W:/src/\"" /std:c++17 /Od /DDEBUG=1 /Z7 /MTd /GR- /EHsc /EHa- %DEBUG_WARNINGS% /WX /permissive- /Femetaprogram.exe W:\src\metaprogram.cpp /link /DEBUG:FULL /opt:ref /incremental:no
REM
REM		if !ERRORLEVEL! neq 0 (
REM			goto metaprogram_failed
REM		)
REM
REM		W:\build\metaprogram.exe
REM
REM		if !ERRORLEVEL! neq 0 (
REM			goto metaprogram_failed
REM		)
REM
REM		cl /nologo /DDATA_DIR="\"W:/data/\"" /DEXE_DIR="\"W:/build/\"" /DSRC_DIR="\"W:/src/\"" /std:c++17 /Od /DDEBUG=1 /Z7 /MTd /GR- /EHsc /EHa- %DEBUG_WARNINGS% /permissive- /FeMeat.exe W:\src\Meat.cpp /link /DEBUG:FULL /opt:ref /incremental:no
REM		goto end
REM
REM		:metaprogram_failed
REM		echo Metaprogram failed.
REM
REM		:end
REM		:

		cl /nologo /DDATA_DIR="\"W:/data/\"" /DEXE_DIR="\"W:/build/\"" /DSRC_DIR="\"W:/src/\"" /std:c++17 /Od /DDEBUG=1 /Z7 /MTd /GR- /EHsc /EHa- %DEBUG_WARNINGS% /permissive- /FeMeat.exe W:\src\Meat_DirectX11.cpp /link %LIBRARIES% /subsystem:WINDOWS /DEBUG:FULL /opt:ref /incremental:no
	)
popd

REM C4201       : "nonstandard extension used : nameless struct/union"
REM C4505       : "'function' : unreferenced local function has been removed"
REM C4101       : "'identifier' : unreferenced local variable"
REM /Z7         : "The /Z7 option produces object files that also contain full symbolic debugging information for use with the debugger."
REM /MTd        : "Defines _DEBUG and _MT. This option also causes the compiler to place the library name LIBCMTD.lib into the .obj file so that the linker will use LIBCMTD.lib to resolve external symbols."
REM /GR         : "Adds code to check object types at run time."
REM /EHsc       : "When used with /EHs, the compiler assumes that functions declared as extern "C" never throw a C++ exception."
REM /EHa        : "Enables standard C++ stack unwinding."
REM /permissive : "Specify standards conformance mode to the compiler."
