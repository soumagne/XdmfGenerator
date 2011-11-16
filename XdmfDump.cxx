/*=========================================================================

  Project                 : XdmfGenerator
  Module                  : XdmfDump.cxx

  Authors:
     John Biddiscombe     Jerome Soumagne
     biddisco@cscs.ch     soumagne@cscs.ch

  Copyright (C) CSCS - Swiss National Supercomputing Centre.
  You may use modify and and distribute this code freely providing
  1) This copyright notice appears on all copies of source code
  2) An acknowledgment appears with any substantial usage of the code
  3) If this code is contributed to any other open source project, it
  must not be reformatted such that the indentation, bracketing or
  overall style is modified significantly.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  This work has received funding from the European Community's Seventh
  Framework Programme (FP7/2007-2013) under grant agreement 225967 “NextMuSE”

=========================================================================*/

#include "XdmfDump.h"

#include "h5dump.h"

#ifdef USE_H5FD_DSM
#include "H5FDdsmManager.h"
#include "H5FDdsm.h"
#endif

//----------------------------------------------------------------------------
XdmfDump::XdmfDump()
{
  this->DsmManager = NULL;
  this->FileName = NULL;
}
//----------------------------------------------------------------------------
XdmfDump::~XdmfDump()
{
  this->SetFileName(NULL);
}
//----------------------------------------------------------------------------
void
XdmfDump::SetDsmManager(H5FDdsmManager *dsmManager)
{
  this->DsmManager = dsmManager;
}
//----------------------------------------------------------------------------
void
XdmfDump::Dump()
{
  const char *argv[4]={"./h5dump", "-f", "dsm", this->FileName};
  std::ostringstream stream;
#ifdef USE_H5FD_DSM
  H5FD_dsm_set_manager(this->DsmManager);
#endif
  H5dump(4, (const char**) argv, stream);
#ifdef USE_H5FD_DSM
  if(this->DsmManager->GetUpdatePiece() == 0) {
#endif
  std::cout << stream.str() << std::endl;
#ifdef USE_H5FD_DSM
  }
#endif
}
//----------------------------------------------------------------------------
void
XdmfDump::DumpLight()
{
  const char *argv[5]={"./h5dump", "-f", "dsm", "-H", this->FileName};
  std::ostringstream stream;
#ifdef USE_H5FD_DSM
  H5FD_dsm_set_manager(this->DsmManager);
#endif
  H5dump(5, (const char**) argv, stream);
#ifdef USE_H5FD_DSM
  if(this->DsmManager->GetUpdatePiece() == 0) {
#endif
  std::cout << stream.str() << std::endl;
#ifdef USE_H5FD_DSM
  }
#endif
}
//----------------------------------------------------------------------------
void
XdmfDump::DumpXML(std::ostringstream &stream)
{
  if (this->DsmManager) {
    const char *argv[8] = {"./h5dump", "-f", "dsm", "-x", "-X", ":", "-H", this->FileName};
#ifdef USE_H5FD_DSM
    H5FD_dsm_set_manager(this->DsmManager);
#endif
    H5dump(8, (const char**) argv, stream);
  } else {
    const char *argv[6] = {"./h5dump", "-x", "-X", ":", "-H", this->FileName};
    H5dump(6, (const char**) argv, stream);
  }
}
//----------------------------------------------------------------------------
