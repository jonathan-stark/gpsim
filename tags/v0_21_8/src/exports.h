#ifndef __GPSIM_EXPORTS_H__
#define __GPSIM_EXPORTS_H__

#ifdef _WIN32
// Microsoft Visual C DLL export and import macros
// for classes exported from gpsim.exe.
// Supported by GCC on Windows.
// Each DLL will need its own versions with xxx_DLL_EXPORT
// defined for the modules that contain the code.
# if defined(GPSIM_DLL_EXPORT)
#  define GPSIM_EXT_CLASS __declspec( dllexport )
#  define GPSIM_EXPORT    __declspec( dllexport )
// #pragma message ("dllexport")
# else
#  define GPSIM_EXT_CLASS __declspec( dllimport )
#  define GPSIM_EXPORT
// #pragma message ("dllimport")
# endif
#else
#  define GPSIM_EXT_CLASS
#  define GPSIM_EXPORT
// #pragma message ("GPSIM_EXT_CLASS defined a nothing")
#endif

class ProgramFileType;
///
///   Exported functions
bool GPSIM_EXPORT IsFileExtension(const char *pszFile, const char *pFileExt);
void GPSIM_EXPORT RegisterProgramFileType(ProgramFileType * pPFT);
#endif
