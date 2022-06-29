/* 
 * cange -- a very small, simple text editor
 * -----------------------------------------------------------------------------
 * Copyright (c) 2022 Angold Wang <wangold4w at gmail dot com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "cange.h"

static struct cangeConfig Config;

int is_separator(int c) {
  return c == '\0' || isspace(c) || strchr(",.()+-/*=~%[];",c) != NULL;
}

/* check whether this the specified row has a multiline comment ending key */
int editorRowHasOpenComment(crow *row) {
  if (row->hl && row->rsize && row->hl[row->rsize-1] == HL_MLCOMMENT &&
      (row->rsize < 2 || (row->renderc[row->rsize-2] != '*' || 
	row->renderc[row->rsize-1] != '/'))) return 1;

  return 0;
}

/* Set every byte of row->hl to the right syntax highlight type */
void editorUpdateSyntax(crow *row) {
  row->hl = realloc(row->hl, row->rsize);
  memset(row->hl, HL_NORMAL, row->rsize); // all normal first

  if (Config.syntax == NULL) return; // no syntax, everything is HL_NORMAL

  int i, prev_sep, in_string, in_comment;
  char *p;
  char **keywords = Config.syntax->keywords;
  char *scs = Config.syntax->single_comment; char *mcs = Config.syntax->multi_comments_beg;
  char *mce = Config.syntax->multi_comments_end;

  /* The first non-space char */
  p = row->renderc;
  i = 0;
  while (*p && isspace(*p)) {
    p++;
    i++;
  }

  prev_sep = 1;   /* Tell the parser if 'i' points to start of the word. */
  in_string = 0;  /* Are we inside "" or '' ? */
  in_comment = 0; /* Are we inside multi-line comment ? */

  /* If the previous line has an open comment, this line starts
   * with an open comment state. */
  if (row->index > 0 && editorRowHasOpenComment(&Config.crow[row->index-1])) {
    in_comment = 1;
  }

  while (*p) {
    // a tokenizer (finite automata)
    // note that we are just focusing on this row

    // handle single comment first 
    // TODO: default 2 symbols for comment, we want it in any size
    if (prev_sep && *p == scs[0] && *(p+1) == scs[1]) {
      // from here to end is a comment 
      memset(row->hl+i, HL_COMMENT, row->size-i);
      return;
    }

    // handle multi line comments
    if (in_comment) {
      row->hl[i] = HL_MLCOMMENT;
      if (*p == mce[0] && *(p+1) == mce[1]) {
	row->hl[i+1] = HL_MLCOMMENT;
	p += 2; i += 2;
	in_comment = 0;
	prev_sep = 1;
	continue;
      } else {
	// already commented
	prev_sep = 0;
	p++; i++;
	continue;
      }
    }

    // handle "" and ''
    if (in_string) {
      row->hl[i] = HL_STRING;
      if (*p == '\\') {
	row->hl[i+1] = HL_STRING;
	p += 2;
	i += 2;
	prev_sep = 0;
	continue;
      }

      if (*p == in_string) in_string = 0; 
      // we define in_string when we met the begin  
      p++; i++;
      continue;
    } else {
      if (*p == '"' || *p == '\'') {
	in_string = *p;
	row->hl[i] = HL_STRING;
	p++; i++;
	prev_sep = 0;
	continue;
      }
    }

    // handle non printable chars 
    if (!isprint(*p)) { // tests for any printing character
      row->hl[i] = HL_NONPRINT;
      p++; i++;
      prev_sep = 0;
      continue;
    }

    // handle numbers
    if ((isdigit(*p) && (prev_sep || row->hl[i-1] == HL_NUMBER)) ||
	(*p == '.' && i > 0 && row->hl[i-1] == HL_NUMBER)) {
      row->hl[i] = HL_NUMBER;
      p++; i++;
      prev_sep = 0;
      continue;
    }

    // handle keywords
    if (prev_sep) {
      int j;
      for (j = 0; keywords[j]; j++) {
	int klen = strlen(keywords[j]);
	int kw2 = keywords[j][klen-1] == '|';
	if (kw2) klen--;

	if (!memcmp(p, keywords[j], klen) && is_separator(*(p+klen))) {
	  // it is a keyword
	  memset(row->hl+i, kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
	  p += klen;
	  i += klen;
	  break;
	}
      }
      if (keywords[j] != NULL) {
	prev_sep = 0;
	continue;
      }
    }

    // then not a special char
    prev_sep = is_separator(*p);
    p++; i++;
  }

  int oc = editorRowHasOpenComment(row);
  if (row->hl_oc != oc && row->index+1 < Config.numrows) {
    // the next row
    // This may recursively affect all the following row in the file
    editorUpdateSyntax(&Config.crow[row->index+1]);
  }

  row->hl_oc = oc;
}

/* Update the rendered version and the syntax highlight of a row */
void editorUpdateRow(crow *crow) {
  unsigned int tabs = 0, nonprint = 0;
  int i, idx;

  // Creating a version of the row we can directly print on the screen,
  // respecting tabs, substituting non printable characters with '?'
  free(crow->renderc);
  for (i = 0; i < crow->size; i++) {
    if (crow->content[i] == TAB) tabs++;
  }

  // calculate the size
  unsigned long long allocsize = (unsigned long long) crow->size + tabs*8 + nonprint*9 + 1;
  if (allocsize > UINT32_MAX) {
    fprintf(stderr, "Some lines of the edited file is too long\n");
    exit(1);
  }

  crow->renderc = malloc(crow->size + tabs * 8 + nonprint*9 + 1);
  idx = 0;

  for (i = 0; i < crow->size; i++) {
    if (crow->content[i] == TAB) {
      crow->renderc[idx++] = ' ';
      while ((idx+1) % 8 != 0) crow->renderc[idx++] = ' ';
    } else {
      crow->renderc[idx++] = crow->content[i];
    }
  }

  printf("%s\n", crow->renderc);

  crow->rsize = idx;
  crow->renderc[idx] = '\0';

  // update the syntax highlighting of the row
  editorUpdateSyntax(crow);
}


void editorInsertRow(int at, char *s, size_t len) {
  if (at > Config.numrows) return;

  Config.crow = realloc(Config.crow, sizeof(crow) * (Config.numrows+1)); 
  // change the size of allocated area

  if (at != Config.numrows) {
    memmove(Config.crow+at+1, Config.crow+at, sizeof(Config.crow[0]) * (Config.numrows-at));
  }

  Config.crow[at].size = len;
  Config.crow[at].content = malloc(len+1); // pointer content
  memcpy(Config.crow[at].content, s, len+1);
  Config.crow[at].hl = NULL;
  Config.crow[at].hl_oc = 0;

  Config.crow[at].renderc = NULL; // init
  Config.crow[at].rsize = 0;
  Config.crow[at].index = at;

  editorUpdateRow(Config.crow + at);
  Config.numrows++;
  Config.dirty++;
}


/* Load the specified file into editor's memory
 * return 0 on success or 1 on error */
int editorOpen(char *filename) {
  FILE *fp;
  Config.filename = filename;

  fp = fopen(filename, "r");
  if (!fp) {
    if (errno != ENOENT) {
      fprintf(stderr, "Opening file");
      exit(1);
    }
    return 1;
  }

  char *line = NULL;
  size_t linecap = 0; // the buffer size
  ssize_t linelen;    // how many bytes are written to line
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    if (linelen && (line[linelen - 1] == '\n' || line[linelen-1] == '\r')) {
      line[--linelen] = '\0';
    }
    // insert this row into config
    editorInsertRow(Config.numrows, line, linelen);
  }

  free(line);
  fclose(fp);
  Config.dirty = 0;
  return 0;
}


/* Try to get the number of cols and rows in the current terminal.
 * Using ioctl(), returns 0 on succress, -1 on error. */
int getWindowSize(int ifd, int ofd, int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 || ws.ws_row == 0) {
    // ioctl failed, just return;
    return -1;
  }

  *cols = ws.ws_col;
  *rows = ws.ws_row;

  return 0;
}


void updateWindowSize(void) {
  if (getWindowSize(STDIN_FILENO, STDOUT_FILENO, &Config.srows, &Config.scols) == -1) {
    fprintf(stderr, "Unable to get the shell screen size (using ioctl)! \n");
    exit(1);
  }
}


// load the configure file, then do initialization
void init(void) {
  Config.cx = 0;
  Config.cy = 0;
  Config.offcol = 0;
  Config.offrow = 0;
  Config.numrows = 0;
  Config.dirty = 0;
  Config.filename = NULL;
  Config.syntax = NULL;
  updateWindowSize();
}


int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: cange <filename> \n");
    exit(1);
  }

  init();
  loadConfig(argv[1], &Config);
  editorOpen(argv[1]);
}
