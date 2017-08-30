 /*
  * File    : utils.cc
  * Project : fix-dldep
  * Author  : ze00
  * Email   : zerozakiGeek@gmail.com
  * Date    : 2017-08-27
  * Module  : 
  * License : MIT
  */
#include <stdio.h>
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/sysconf.h>
#include "../inc/utils.hpp"
using namespace std;
dataBaseType recursiveFindLib(dataBaseType &db,string &&location) {
  DIR *dp = opendir(location.c_str());
  if(dp == nullptr) {
    fprintf(stderr,"Open '%s' appear an error '%s'\n",location.c_str(),strerror(errno));
    return db;
  }
  struct dirent *ent;
  while((ent = readdir(dp))) {
    std::string s(location + "/" + ent->d_name);
    if(ent->d_type & DT_DIR && strcmp(ent->d_name,".") && strcmp(ent->d_name,"..")) {
      recursiveFindLib(db,move(s));
    } else if(rcmp(s,".so")) {
      db.insert(make_pair<string,dlinfo>(move(s),dlinfo()));
    }
  }
  return db;
}
bool rcmp(const string &l,const char *r) {
  size_t rlen = strlen(r),llen = l.size();
  if(llen < rlen)
    return false;
  return l.substr(llen - rlen,rlen) == r;
}
size_t getfilesz(const string &file) {
  static struct stat st;
  if(stat(file.c_str(),&st) != -1) {
    return st.st_size;
  }
  return -1;
}
void cleanUp(dlinfo &info) {
  munmap(info.addr,info.memsz);
  close(info.fd);
}
void *getResource(const string &file,dlinfo &info) {
  size_t pagesize = sysconf(_SC_PAGESIZE);
  size_t filesz = getfilesz(file);
  size_t memsz = filesz + (pagesize - (filesz & pagesize - 1) & pagesize - 1);
  int fd = open(file.c_str(),O_RDONLY);
  if(fd == -1) {
    fprintf(stderr,"open '%s' failed,cause '%s'\n",file.c_str(),strerror(errno));
    return nullptr;
  }
  void *handle = mmap(nullptr,memsz,PROT_READ,MAP_PRIVATE,fd,0);
  if(handle == RCAST(void*,0xffffffff)) {
    fprintf(stderr,"'%s' mmap failed,cause '%s'\n",file.c_str(),strerror(errno));
    return nullptr;
  }
  info.addr = handle;
  info.fd = fd;
  info.memsz = memsz;
  ++info.cnt;
  return handle;
}
const char *getSectionNameTable(byte *rawHandle) {
  Elfw(Ehdr) *hdr = RCAST(Elfw(Ehdr)*,rawHandle);
  return RCAST(const char *,rawHandle + RCAST(Elfw(Shdr)*,rawHandle + hdr->e_shoff + hdr->e_shentsize * hdr->e_shstrndx)->sh_offset);
}
const char *getSectionName(byte *rawHandle,Elfw(Shdr) *sec) {
  return getSectionNameTable(rawHandle) + sec->sh_name;
}
Elfw(Dyn) *fillDynamic(dlinfo &info) {
  byte *rawHandle = RCAST(byte*,info.addr);
  Elfw(Ehdr) *hdr = RCAST(Elfw(Ehdr*),rawHandle);
  Elfw(Shdr) *sec = RCAST(Elfw(Shdr)*,rawHandle + hdr->e_shoff);
  Elfw(Dyn) *dyn = nullptr;
  Elfw(Sym) *dynsym = nullptr;
  Elfw(Rel) *rel = nullptr;
  Elfw(Rel) *plt = nullptr;
  const char *dynstr = nullptr;
  for(size_t i = 0;i != hdr->e_shnum;++i) {
    if(sec->sh_type == SHT_DYNAMIC) {
      dyn = RCAST(Elfw(Dyn)*,rawHandle + sec->sh_offset);
      info.dynsize = sec->sh_size;
    } else if(sec->sh_type == SHT_STRTAB && strcmp(getSectionName(rawHandle,sec),".dynstr") == 0) {
      dynstr = RCAST(const char *,rawHandle + sec->sh_offset);
    } else if(sec->sh_type == SHT_DYNSYM) {
      dynsym = RCAST(Elfw(Sym)*,rawHandle + sec->sh_offset);
      info.dynsymsize = sec->sh_size;
    } else if(sec->sh_type == SHT_REL && strcmp(getSectionName(rawHandle,sec),".rel.dyn") == 0) {
      rel = RCAST(Elfw(Rel)*,rawHandle + sec->sh_offset);
      info.dynrelsize = sec->sh_size;
    } else if(sec->sh_type == SHT_REL && strcmp(getSectionName(rawHandle,sec),".rel.plt") == 0) {
      plt = RCAST(Elfw(Rel)*,rawHandle + sec->sh_offset);
      info.relpltsize = sec->sh_size;
    }
    ++sec;
  }
  info.dynamic = dyn;
  info.dynstr = dynstr;
  info.dynsym = dynsym;
  info.dynrel = rel;
  info.relplt = plt;
  return dyn;
}
void dumpNeeded(dataBaseType::value_type &v) {
  dlinfo &info = v.second;
  Elfw(Dyn) *pd = info.dynamic;
  size_t maxIndex = info.dynsize / sizeof(*pd);
  for(size_t i = 0;i < maxIndex;++i) {
    if(pd->d_tag == DT_NEEDED) {
      info.dldeps.push_back(sonamewrap{info.dynstr + pd->d_un.d_val,0});
    }
    ++pd;
  }
}
void dumpRelocate(const string &rp,Elfw(Sym) *dlsym,Elfw(Rel) *rel,const char *symtab,size_t relsize) {
  Elfw(Word) idx;
  Elfw(Word) type;
  Elfw(Sym) *sym = nullptr;
  const char *name;
  size_t maxIndex = relsize / sizeof(*rel);
  for(size_t i = 0;i < maxIndex;++i) {
    idx = ELFW(R_SYM)(rel->r_info);
    type = ELFW(R_TYPE)(rel->r_info);
    if(idx != 0) {
      sym = &dlsym[idx];
      name = symtab + sym->st_name;
      if(sym->st_size == 0 && sym->st_value == 0 && ELFW(ST_BIND)(sym->st_info) != STB_WEAK && strcmp(name,"__cxa_finalize") && strcmp(name,"__cxa_atexit"))
#warning TODO:Implement this
        ;
    }
    ++rel;
  }
}
void dumpNeededFC(dataBaseType::value_type &v) {
  dlinfo &info = v.second;
  const string &s = v.first;
  if(info.dynrel)
    dumpRelocate(s,info.dynsym,info.dynrel,info.dynstr,info.dynrelsize);
  if(info.relplt)
    dumpRelocate(s,info.dynsym,info.relplt,info.dynstr,info.relpltsize);
}
void checkDependent(dataBaseType &sdb,vector<sonamewrap> &libs,const string &target) {
  for(auto &x : libs) {
    for(auto const &y : sdb) {
      if(rcmp(y.first,x.so))
        x.mark = 1;
    }
    if(x.mark == 0)
      printf("%s [ %s ] not found\n",target.c_str(),x.so);
  }
}
void fixDependent(dataBaseType &sdb,const dataBaseType &edb) {
  for(auto &x : sdb) {
    const string &lib = x.first;
    dlinfo &inf = x.second;
    if(getResource(lib,inf) != nullptr) {
      fillDynamic(inf);
      dumpNeeded(x);
      if(!inf.dldeps.empty()) {
        checkDependent(sdb,inf.dldeps,lib);
        dumpNeededFC(x);
      }
      cleanUp(inf);
    }
  }
}
