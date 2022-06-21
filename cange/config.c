#include "cange.h"

// Supported language
/* C */
char *C_postfix[] = {".c", ".h", NULL};
char *CHL_keywords[] = {
  /* C Keywords */ "auto","break","case","continue","default","do","else","enum",
  "extern","for","goto","if","register","return","sizeof","static",
  "struct","switch","typedef","union","volatile","while","NULL",

  /* C types */
  "int|","long|","double|","float|","char|","unsigned|","signed|",
  "void|","short|","auto|","const|","bool|",NULL
};

char *CPP_postfix[] = {".cpp", ".hpp", ".cc", NULL};
char *CPP_keywords[] = {
  /* C++ Keywords */
  "alignas","alignof","and","and_eq","asm","bitand","bitor","class",
  "compl","constexpr","const_cast","deltype","delete","dynamic_cast",
  "explicit","export","false","friend","inline","mutable","namespace",
  "new","noexcept","not","not_eq","nullptr","operator","or","or_eq",
  "private","protected","public","reinterpret_cast","static_assert",
  "static_cast","template","this","thread_local","throw","true","try",
  "typeid","typename","virtual","xor","xor_eq",

  /* C++ types */
  "int|","long|","double|","float|","char|","unsigned|","signed|",
  "void|","short|","auto|","const|","bool|", "vector|", NULL
};

struct cangeSyntax HL[] = {
  {
    "The C Programming Language",
    /* C */
    C_postfix,
    CHL_keywords,
    "//","/*","*/",
  },
  
  {
    "The C++ Programming Language",
    /* C++ */
    CPP_postfix,
    CPP_keywords,
    "//","/*","*/",
  },
};

#define HL_ENTRIES (sizeof(HL) / sizeof(HL[0]))

void scheme(char* filename, struct cangeConfig *conf) {
  for (unsigned int i = 0; i < HL_ENTRIES; i++) {
    struct cangeSyntax *sche = HL+i;
    unsigned int j = 0;
    while (sche->filetype[j]) {
      char *p;
      int len = strlen(sche->filetype[j]);
      if ((p = strstr(filename, sche->filetype[j])) != NULL) {
	if (p[len] == '\0') {
	  conf->syntax = sche;
	  printf("The scheme is: %s\n", sche->name);
	  return;
	}
      }
      j++;
    }
  }
}

void loadConfig(char* filename, struct cangeConfig *conf) {
  // #1 select the syntax highlight scheme depending on the filename
  scheme(filename, conf);
};

