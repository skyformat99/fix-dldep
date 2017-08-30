 /*
  * File    : inc/defs.hpp
  * Project : fix-dldep
  * Author  : ze00
  * Email   : zerozakiGeek@gmail.com
  * Date    : 2017-08-27
  * Module  : 
  * License : MIT
  */
#ifndef __DEFS__HPP
#define __DEFS__HPP
#include <map>
#include <string>
#include <vector>
#include <utility>
#define STR(x) std::string(x)
#if BIT == 32
#define Elfw(type) Elf32_##type
#define ELFW(suff) ELF32_##suff
#else
#define Elfw(type) Elf64_##type
#define ELFW(suff) ELF64_##suff
#endif
#define RCAST(type,val) reinterpret_cast<type>(val)
struct sonamewrap {
  const char *so;
  unsigned char mark;
};
struct dlinfo {
  size_t memsz;
  size_t cnt = 0;
  int fd;
  void *addr;
  Elfw(Dyn) *dynamic;
  size_t dynsize;
  Elfw(Sym) *dynsym;
  size_t dynsymsize;
  Elfw(Rel) *dynrel;
  size_t dynrelsize;
  Elfw(Rel) *relplt;
  size_t relpltsize;
  const char *dynstr;
  const char *strtab;
  std::vector<sonamewrap> dldeps;
};
typedef std::map<std::string,dlinfo> dataBaseType;
typedef unsigned char byte;
#endif //__DEFS__HPP
