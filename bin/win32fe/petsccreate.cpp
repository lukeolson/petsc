/* $Id:$ */
#include <map>
#include <string>
#include <iostream>
#include "petscfe.h"
#include "petscclfe.h"
#include "petscbccfe.h"
#include "petscccfe.h"
#include "petsclibfe.h"
#include "petsctlibfe.h"
#include "petscarfe.h"

using namespace std;

namespace PETScFE {

  void CreateCL(tool *&Tool) {Tool = new cl;}
  void CreateBCC(tool *&Tool) {Tool = new bcc;}
  void CreateCC(tool *&Tool) {Tool = new cc;}
  void CreateLIB(tool *&Tool) {Tool = new lib;}
  void CreateTLIB(tool *&Tool) {Tool = new tlib;}
  void CreateAR(tool *&Tool) {Tool = new ar;}

  void tool::Create(tool *&Tool,char *argv) {
    string KnownTools = "cl.df.bcc32.cc.lib.tlib.ar";
    string arg = argv;
    if (KnownTools.find(arg)!=string::npos) {
      map<string,void (*)(PETScFE::tool*&)> CreateTool;
      CreateTool["cl"] = PETScFE::CreateCL;
      CreateTool["df"] = PETScFE::CreateCL;
      CreateTool["bcc32"] = PETScFE::CreateBCC;
      CreateTool["cc"] = PETScFE::CreateCC;
      CreateTool["lib"] = PETScFE::CreateLIB;
      CreateTool["tlib"] = PETScFE::CreateTLIB;
      CreateTool["ar"] = PETScFE::CreateAR;
      
      CreateTool[(string)argv](Tool);
    } else {
      cout << "Unknown Tool" << endl;
      cout << "Error: 1" << endl;
      Tool = NULL;
    }
  }

};

