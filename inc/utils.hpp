 /*
  * File    : inc/utils.hpp
  * Project : fix-dldep
  * Author  : ze00
  * Email   : zerozakiGeek@gmail.com
  * Date    : 2017-08-27
  * Module  : 
  * License : MIT
  */
#ifndef __UTILS__HPP
#define __UTILS__HPP
#include <elf.h>
#include "defs.hpp"
dataBaseType recursiveFindLib(dataBaseType &,std::string &&);
bool rcmp(const std::string &,const char *);
void fixDependent(dataBaseType &,const dataBaseType &);
size_t getfilesz(const std::string &);
void cleanUp(dataBaseType::value_type &);
void *getResource(dataBaseType::value_type &);
const char *getSectionNameTable(byte *);
const char *getSectionName(byte *,Elfw(Shdr) *);
Elfw(Dyn) *fillDynmaic(dlinfo &);
#endif //__UTILS__HPP
