#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DEBUG 1;

/* Syntax highlight types */
#define HL_NORMAL 0
#define HL_NONPRINT 1
#define HL_COMMENT 2     /* Single line comment. */
#define HL_MLCOMMENT 3   /* Multi-line comment. */
#define HL_KEYWORD1 4
#define HL_KEYWORD2 5
#define HL_STRING 6
#define HL_NUMBER 7
#define HL_MATCH 8       /* Search match. */

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
  int rsize;       // size of the rendered row
  char *content;   // row content
  char *renderc;   // content rendered for screen (TABs)

  unsigned char *hl;  // Syntax highlight type for each character in render.
  int hl_oc;          // Row had open comment at end in last syntax highlight check
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


enum KEY_ACTION{
  KEY_NULL = 0,       /* NULL */
  CTRL_C = 3,         /* Ctrl-c */
  CTRL_D = 4,         /* Ctrl-d */
  CTRL_F = 6,         /* Ctrl-f */
  CTRL_H = 8,         /* Ctrl-h */
  TAB = 9,            /* Tab */
  CTRL_L = 12,        /* Ctrl+l */
  ENTER = 13,         /* Enter */
  CTRL_Q = 17,        /* Ctrl-q */
  CTRL_S = 19,        /* Ctrl-s */
  CTRL_U = 21,        /* Ctrl-u */
  ESC = 27,           /* Escape */
  BACKSPACE =  127,   /* Backspace */
  /* The following are just soft codes, not really reported by the
   * terminal directly. */
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};



//
// config.c
//

void loadConfig(char *filename, struct cangeConfig *conf);


