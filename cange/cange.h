#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DEBUG 1;

/* the syntax of current open file */
struct cangeSyntax {
  // each char array should end with a explicit NULL
  char *name;
  char **filetype; // the type of current file, based on its postfix
  char **keywords; // the highlightable keywords
  char *single_comment;
  char *multi_comments_beg;
  char *multi_comments_end;
};

// a single line of the file that we are currently editing.
typedef struct crow {
  int index;       // current row index in the file, zero-based.
  int size;        // size of the row
  char *content;   // row content
  char *renderc;   // content rendered for screen (TABs)
} crow;

struct cangeConfig {
  int cx, cy;      // Cursor's current position
  int offrow;      // Offset of row displayed
  int offcol;      // Offset of col displayed

  int srows;       // # of rows that we can show
  int scols;       // # of cols that we can show

  int numrows;     // total rows of this file

  int dirty;       // 1 if there are some unsaved data
  char *filename;  // open filename

  struct cangeSyntax *syntax;   // Current syntax highlight, or NULL
  crow *crow;      // rows
};



//
// config.c
//

void loadConfig(char *filename, struct cangeConfig *conf);


