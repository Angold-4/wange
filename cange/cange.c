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
}
