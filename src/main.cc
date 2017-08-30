 /*
  * File    : main.cc
  * Project : fix-dldep
  * Author  : ze00
  * Email   : zerozakiGeek@gmail.com
  * Date    : 2017-08-27
  * Module  : 
  * License : MIT
  */
#include <stdio.h>
#include <stdlib.h>
#include "../inc/utils.hpp"
int main(int argc,char **argv) {
  if(argc == 1) {
    fprintf(stderr,"./fix-dldep [source location] [ext location]\n");
    exit(-1);
  }
  const char *sourceDir = nullptr,*extDir = nullptr;
  sourceDir = argv[1];
  dataBaseType sourceLib,extLib;
  if(argc > 2) {
    extDir = argv[2];
    recursiveFindLib(extLib,STR(extDir));
  }
  recursiveFindLib(sourceLib,STR(sourceDir));
  fixDependent(sourceLib,extLib);
  return 0;
}
