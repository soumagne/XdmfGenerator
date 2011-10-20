/*=========================================================================

  Project                 : XdmfGenerator
  Module                  : XdmfDump.h

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
#ifndef __XdmfDump_h
#define __XdmfDump_h

#include "XdmfGeneratorconfig.h"
#include "XdmfObject.h"

class H5FDdsmManager;

#include <cstring>
#include <sstream>

class XDMF_EXPORT XdmfDump : public XdmfObject {

    public :
        XdmfDump();
        ~XdmfDump();

        XdmfSetStringMacro(FileName);
        XdmfGetStringMacro(FileName);

        void Dump();
        void DumpLight();
        void DumpXML(std::ostringstream &);

        void SetDsmManager(H5FDdsmManager* _arg);

    protected:
        H5FDdsmManager *DsmManager;
        XdmfString      FileName;
};

#endif // __XdmfDump_h
