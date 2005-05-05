#include "../config.h"
#include "program_files.h"
#include "pic-processor.h"
#include "cod.h"


/**
  * RegisterProgramFileType
  * Exported function for external modules to register their
  * file types.
  */

void GPSIM_EXPORT RegisterProgramFileType(ProgramFileType * pPFT) {
  ProgramFileTypeList::GetList().push_back(pPFT);
}

/**
  * ProgramFileTypeList
  * Singleton class to manage the many (as of now three) file types.
  */
ProgramFileTypeList * ProgramFileTypeList::s_ProgramFileTypeList =
  new ProgramFileTypeList();
// We will instanciate g_HexFileType and g_CodFileType here to be sure
// they are instanciated after s_ProgramFileTypeList. The objects will 
// move should the PIC code moved to its own external module.
static PicHexProgramFileType g_HexFileType;
static PicCodProgramFileType g_CodFileType;

ProgramFileTypeList &ProgramFileTypeList::GetList() {
  return *s_ProgramFileTypeList;
}

ProgramFileTypeList::ProgramFileTypeList() {
}

bool ProgramFileTypeList::LoadProgramFile(Processor **pProcessor,
                                          const char *pFilename,
                                          FILE *pFile) {
  iterator it;
  iterator itEnd = end();
  for(it = begin(); it != itEnd; it++) {
    fseek(pFile, 0, SEEK_SET);
    if((*it)->LoadProgramFile(pProcessor, pFilename, pFile)
      == ProgramFileType::SUCCESS) {
      return true;
    }
  }
  return false;
}

///
/// ProgramFileBuf
/// Used to wrap the FILE pointer to the program file
/// for libraries that use istreams.
/////////////////////////////////////////////////////

ProgramFileBuf::ProgramFileBuf(FILE *pFile) {
  m_pFile = pFile;
  setg(m_Buffer + 4, m_Buffer + 4, m_Buffer + 4);
}

ProgramFileBuf::int_type ProgramFileBuf::underflow( ) {
  char z = 0;
  if(gptr() < egptr()) {
    return *gptr();
  }
  int numPutback;
  numPutback = gptr() - eback();
  if (numPutback > 4) {
    numPutback = 4;
  }
  std::memcpy (m_Buffer+(4-numPutback), gptr() - numPutback, numPutback);

  int num;
  if((num = ::fread((void*)( m_Buffer + 4), 1, m_iBufferSize - 4, m_pFile)) <= 0) {
    printf(strerror(errno));
    return (int_type )traits_type::eof();
  }
  setg(m_Buffer + (4 - numPutback),
       m_Buffer + 4, m_Buffer + 4 + num);
  return *gptr();
}

streamsize ProgramFileBuf::xsgetn(
  char_type *_Ptr, streamsize _Count) {
  return ::fread(_Ptr, _Count, 1, m_pFile);
}
