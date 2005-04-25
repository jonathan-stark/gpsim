rem This file was made to be run from the Visual Studio build
echo off
set TargetDir=%1
echo %0 %1
if not exist %TargetDir%\popt1.dll copy ..\..\popt\bin\popt1.dll %TargetDir%
if not exist %TargetDir%\libintl-2.dll copy ..\..\intl\bin\libintl-2.dll %TargetDir%
if not exist %TargetDir%\libiconv-2.dll copy ..\..\iconv\bin\libiconv-2.dll %TargetDir%
if not exist %TargetDir%\libglib-2.0-0.dll copy ..\..\glib\bin\libglib-2.0-0.dll %TargetDir%
if not exist %TargetDir%\iconv.dll copy ..\..\iconv-1.9\bin\iconv.dll %TargetDir%
if not exist %TargetDir%\intl.dll copy ..\..\gettext\bin\intl.dll %TargetDir%
if not exist %TargetDir%\libgobject-2.0-0.dll copy ..\..\glib\bin\libgobject-2.0-0.dll %TargetDir%
if not exist %TargetDir%\libgthread-2.0-0.dll copy ..\..\glib\bin\libgthread-2.0-0.dll %TargetDir%
if not exist %TargetDir%\libgmodule-2.0-0.dll copy ..\..\glib\bin\libgmodule-2.0-0.dll %TargetDir%
if not exist %TargetDir%\libgdk-win32-2.0-0.dll copy "..\..\gtk+\bin\libgdk-win32-2.0-0.dll" %TargetDir%
if not exist %TargetDir%\libgdk_pixbuf-2.0-0.dll copy "..\..\gtk+\bin\libgdk_pixbuf-2.0-0.dll" %TargetDir%
if not exist %TargetDir%\libpango-1.0-0.dll copy ..\..\pango\bin\libpango-1.0-0.dll %TargetDir%
if not exist %TargetDir%\libpangowin32-1.0-0.dll copy ..\..\pango\bin\libpangowin32-1.0-0.dll %TargetDir%
if not exist %TargetDir%\libgtk-win32-2.0-0.dll copy "..\..\gtk+\bin\libgtk-win32-2.0-0.dll" %TargetDir%
if not exist %TargetDir%\libatk-1.0-0.dll copy "..\..\atk\bin\libatk-1.0-0.dll" %TargetDir%

