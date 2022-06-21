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
