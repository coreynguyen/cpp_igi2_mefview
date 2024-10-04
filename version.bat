@echo off

rem Configurable Variables
set "appname=MEF View"
set "appver=v0.1"

rem System-Generated Variables for Date and Time
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "dt=%%a"
set "YY=%dt:~2,2%" & set "YYYY=%dt:~0,4%" & set "MM=%dt:~4,2%" & set "DD=%dt:~6,2%"
set "HH=%dt:~8,2%" & set "Min=%dt:~10,2%" & set "Sec=%dt:~12,2%" & set "MS=%dt:~15,3%"
set "appdate=%YYYY%-%MM%-%DD%"

rem Construct the Application Title
set "apptitle=%appname% %appver% (Build: %appdate%)"

rem Write to version.h
>"%~dp0\\include\version.h" echo #ifndef appver_h
>>"%~dp0\\include\version.h" echo #define appver_h
>>"%~dp0\\include\version.h" echo constexpr char const* appname = "%appname%";
>>"%~dp0\\include\version.h" echo constexpr char const* appver = "%appver%";
>>"%~dp0\\include\version.h" echo constexpr char const* appdate = "%appdate%";
>>"%~dp0\\include\version.h" echo constexpr char const* apptitle = "%apptitle%";
>>"%~dp0\\include\version.h" echo #endif
