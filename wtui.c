/* wtui - A simple ncurses extension for JimTcl, the embeddable 
 *        Tcl interpreter
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED IN THE NAME OF JESUS CHRIST ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * JIM TCL PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the Jim Tcl Project.
 */

#define _XOPEN_SOURCE 700
#include <wchar.h>

#include <jim.h>

#define NCURSES_WIDECHAR 1
#include <ncursesw/panel.h>
#include <ncursesw/menu.h>
#include <ncursesw/form.h>

#include <locale.h>
#include <string.h>
#include <errno.h>

#define MAX_CMD_NAME_LEN 64

#define RETURN_IF_ERR(cond, str, ...) if (cond) { Jim_SetResultFormatted(interp, (str), __VA_ARGS__); return JIM_ERR; }

#define RETURN_IF_NCERR(status) if (check_error((status), interp, cmd, subcmd) == JIM_ERR) { return JIM_ERR; }

typedef struct {
  const char *name;
  cchar_t *macro;
} WacsMacro;

typedef struct {
  const char *name;
  FIELDTYPE *macro;
} FieldTypeMacro;

typedef struct {
 const char *name;
 NCURSES_COLOR_T macro;
} ColorMacro;

typedef struct {
  int errcode;
  const char *errmsg;
} NcursesError;

typedef struct {
  const char *name;
  attr_t macro;
} AttrMacro;

typedef struct {
  const char *name;
  int macro;
} IntMacro;

typedef struct {
  const char *name;
  unsigned int macro;
} UIntMacro;

typedef int (*SubCmdFunc)(void *, const char *, const char *, Jim_Interp *, int, Jim_Obj *const *);

typedef struct {
  const char *name;
  SubCmdFunc func;
} SubCmd;

static unsigned int hash(const char *key) {
  /* one tcl hash algorithm */
  unsigned int hash = 0;
  while (*key) {
    hash = (hash << 5) - hash + (unsigned int)*key;
    ++key;
  }
  return hash;
}

static int initialized = 0;
static bool noblock = 0;
static int ncolors = 1;
static int ncolorpairs = 0;
/* these file visible variables are needed because WacsMacro contains non-constant macros which have to be initialized */
static int WacsMacroNRows;
static int WacsMacroNCols;
static WacsMacro *WacsMacroArray = NULL;

static cchar_t *getWacsMacroVal(const char *key, WacsMacro *array, int nrows, int ncols) {
  int flatidx = (hash(key) % nrows) * ncols;
  for (int col = 0; col < ncols; ++col) {
    if (array[flatidx].name == NULL) 
      return NULL;
    else if (strcmp(key, array[flatidx].name) == 0)
      return array[flatidx].macro;
    ++flatidx;
  }
  return NULL;
}

/* these file visible variables are needed because FTMacro contains macros which are pointers */
static int FTMacroNCols;
static int FTMacroNRows;
static FieldTypeMacro *FTMacro = NULL;

static FIELDTYPE *getFTMacroVal(const char *key, FieldTypeMacro *array, int nrows, int ncols) {
  int flatidx = (hash(key) % nrows) * ncols;
  for (int col = 0; col < ncols; ++col) {
    if (array[flatidx].name == NULL) 
      return NULL;
    else if (strcmp(key, array[flatidx].name) == 0)
      return array[flatidx].macro;
    ++flatidx;
  }
  return NULL;
}

static NCURSES_COLOR_T getColorMacroVal(const char *key, ColorMacro *array, int nrows, int ncols) {
  int flatidx = (hash(key) % nrows) * ncols;
  for (int col = 0; col < ncols; ++col) {
    if (array[flatidx].name == NULL) 
      return (NCURSES_COLOR_T)(-1);
    else if (strcmp(key, array[flatidx].name) == 0)
      return array[flatidx].macro;
    ++flatidx;
  }
  return (NCURSES_COLOR_T)(-1);
}

static attr_t getAttrMacroVal(const char *key, AttrMacro *array, int nrows, int ncols) {
  int flatidx = (hash(key) % nrows) * ncols;
  for (int col = 0; col < ncols; ++col) {
    if (array[flatidx].name == NULL) 
      return (attr_t)-1;
    else if (strcmp(key, array[flatidx].name) == 0)
      return array[flatidx].macro;
    ++flatidx;
  }
  return (attr_t)-1;
}

static int getIntMacroVal(const char *key, IntMacro *array, int nrows, int ncols) {
  int flatidx = (hash(key) % nrows) * ncols;
  for (int col = 0; col < ncols; ++col) {
    if (array[flatidx].name == NULL) 
      return -1;
    else if (strcmp(key, array[flatidx].name) == 0)
      return array[flatidx].macro;
    ++flatidx;
  }
  return -1;
}

static int getUIntMacroVal(const char *key, UIntMacro *array, int nrows, int ncols) {
  int flatidx = (hash(key) % nrows) * ncols;
  for (int col = 0; col < ncols; ++col) {
    if (array[flatidx].name == NULL) 
      return (unsigned int)-1;
    else if (strcmp(key, array[flatidx].name) == 0)
      return array[flatidx].macro;
    ++flatidx;
  }
  return (unsigned)-1;
}

static SubCmdFunc getSubCmd(const char *key, SubCmd *array, int nrows, int ncols) {
  int flatidx = (hash(key) % nrows) * ncols;
  for (int col = 0; col < ncols; ++col) {
    if (array[flatidx].name == NULL) 
      return NULL;
    else if (strcmp(key, array[flatidx].name) == 0)
      return array[flatidx].func;
    ++flatidx;
  }
  return NULL;
}

static int check_error(int status, Jim_Interp *interp, const char *cmd, const char *subcmd) {
  static NcursesError StaticErrors[] = {
    {ERR, "ERR"},
    {E_BAD_ARGUMENT, "E_BAD_ARGUMENT"},
    {E_BAD_STATE, "E_BAD_STATE"},
    {E_NO_MATCH, "E_NO_MATCH"},
    {E_NO_ROOM, "E_NO_ROOM"},
    {E_NOT_CONNECTED, "E_NOT_CONNECTED"},
    {E_NOT_POSTED, "E_NOT_POSTED"},
    {E_NOT_SELECTABLE, "E_NOT_SELECTABLE"},
    {E_POSTED, "E_POSTED"},
    {E_REQUEST_DENIED, "E_REQUEST_DENIED"},
    {E_UNKNOWN_COMMAND, "E_UNKNOWN_COMMAND"},
    {E_CONNECTED, "E_CONNECTED"},
    {E_INVALID_FIELD, "E_INVALID_FIELD"},
    {E_CURRENT, "E_CURRENT"},
  };  
  static int StaticErrorsNRows = sizeof(StaticErrors) / sizeof(NcursesError);
  if (status == OK) return JIM_OK;
  const char *errmsg = NULL;
  int i = 0;
  if (status == E_SYSTEM_ERROR) {
    if (errno == -1) {
      errmsg = "E_SYSTEM_ERROR, no further info available";
    } else {
      errmsg = strerror(errno);
    }
  } else {
    for (; i < StaticErrorsNRows; ++i) {
      if (status == StaticErrors[i].errcode) {
        errmsg = StaticErrors[i].errmsg;
        break;
      }
    }
  }
  if (i >= StaticErrorsNRows) errmsg = "unknown error code";
  if (subcmd)
    Jim_SetResultFormatted(interp, "%s %s: error %s", cmd, subcmd, errmsg);
  else
    Jim_SetResultFormatted(interp, "%s: error %s", cmd, errmsg);
  return JIM_ERR;    
}

static int z_delete(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wf_newform(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wm_newmenu(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_newpanel(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);

static int wt_colordetail(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wt_colorpair(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wt_colwidth(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wt_cursor(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wt_recolor(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wt_size(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wt_style(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wt_update(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);

static int TuiCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  /* Special order required, get details using commands: 
   *     cat wtui.c | ./ckhash code '\{"([a-z]+)",' 0.7 1.5 > test1
   *     cat wtui.c | ./ckhash stats '\{"([a-z]+)",' 0.7 1.5 
   * Then z_ or wt_ or wp_ or wm_ or wf_ need to be manually prepended to the function names */
  static SubCmd TuiSubcmds[] = {
    {"cursor", wt_cursor}, {"newpanel", wp_newpanel},
    {"update", wt_update}, {NULL, 0}, 
    {"newform", wf_newform}, {NULL, 0}, 
    {"style", wt_style}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"delete", z_delete}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"colwidth", wt_colwidth}, {"recolor", wt_recolor},
    {"size", wt_size}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"colorpair", wt_colorpair}, {NULL, 0}, 
    {"colordetail", wt_colordetail}, {NULL, 0}, 
    {"newmenu", wm_newmenu}, {NULL, 0}, 
  };
#undef NCOLS
#define NCOLS 2
  static int TuiSubcmdsNCols = NCOLS;
  static int TuiSubcmdsNRows = sizeof(TuiSubcmds) / sizeof(SubCmd) / NCOLS;
  if (argc < 2) {
    Jim_WrongNumArgs(interp, 1, argv, "subcommand ?...?");
    return JIM_ERR;
  }
  const char *cmd = Jim_String(argv[0]);
  const char *subcmd = Jim_String(argv[1]);
  SubCmdFunc func = getSubCmd(subcmd, TuiSubcmds, TuiSubcmdsNRows, TuiSubcmdsNCols);
  RETURN_IF_ERR(! func, "%s: unknown subcommand \"%s\"", cmd, subcmd);
  return func(Jim_CmdPrivData(interp), cmd, subcmd, interp, argc - 2, argv + 2);
}

static void TuiDeleteCmd(Jim_Interp *interp, void *privData) {
  (void)interp;
  (void)privData;
  endwin();
  if (WacsMacroArray != NULL) {
    Jim_Free(WacsMacroArray);
    WacsMacroArray = NULL;
  }
  if (FTMacro != NULL) {
    Jim_Free(FTMacro);
    FTMacro = NULL;
  }
  initialized = 0;
}

static int TuiCreateCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 1 && argc != 2 && argc != 3) {
    Jim_WrongNumArgs(interp, 1, argv, "?ndelay? ?escdelayms?");
    return JIM_ERR;
  }
  const char *cmd = Jim_String(argv[0]);
  RETURN_IF_ERR(initialized, "%s error: already active", cmd);
  long escdelay = 25; // ms
  /* static int */ noblock = FALSE;
  if (argc == 2) {
    if (Jim_GetLong(interp, argv[1], &escdelay) != JIM_OK) {
      const char *ndelay = Jim_String(argv[1]);
      RETURN_IF_ERR(strcmp("ndelay", ndelay) != 0, "%s: unknown flag \"%s\" expected \"ndelay\"", cmd, ndelay);
      /* static int */ noblock = TRUE;
    }
  } else if (argc == 3) {
    const char *ndelay = Jim_String(argv[1]);
    RETURN_IF_ERR(strcmp("ndelay", ndelay) != 0, "%s: unknown flag \"%s\" expected \"ndelay\"", cmd, ndelay);
    /* static int */ noblock = TRUE;
    RETURN_IF_ERR(Jim_GetLong(interp, argv[2], &escdelay) != JIM_OK, "%s: escdelay value \"%s\" should be integer", cmd, Jim_String(argv[2]))
  }
  setlocale(LC_ALL, ""); /* for correct char and UTF8 rendering). */
  RETURN_IF_ERR(initscr() == NULL, "%s: initialization failed (?tty problem?)", cmd);
  /* The following macros must not be used before initscr() is called. 
  * The array below (which trades extra memory for speed) is created by:
  *     cat wtui.c | ./ckhash code '\{"(WACS_[A-Z_0-9]+)",' 0.7 1.5 > test1
  * 0.7 and 1.5 indicate to check "2-D" arrays with nRows between 0.7 * N and 
  * 1.5 * N where N is the number of macros. I.e. 0.7 relates to minRows and 
  * 1.5 relates to maxRows. In actuality, the array is not "2-D", but 
  * 1-dimensional. nRows and nCols obtained from output of:
  *     cat wtui.c | ./ckhash stats '\{"(WACS_[A-Z_0-9]+)",' 0.7 1.5 
  * run ./ckhash for usage. */
  WacsMacro tmp[] = {
    {NULL, 0}, {NULL, 0},
    {"WACS_LTEE", WACS_LTEE}, {NULL, 0}, 
    {"WACS_D_HLINE", WACS_D_HLINE}, {NULL, 0}, 
    {"WACS_STERLING", WACS_STERLING}, {"WACS_ULCORNER", WACS_ULCORNER},
    {"WACS_D_LTEE", WACS_D_LTEE}, {NULL, 0}, 
    {"WACS_D_LLCORNER", WACS_D_LLCORNER}, {"WACS_VLINE", WACS_VLINE},
    {"WACS_LLCORNER", WACS_LLCORNER}, {NULL, 0}, 
    {"WACS_RTEE", WACS_RTEE}, {NULL, 0}, 
    {"WACS_LARROW", WACS_LARROW}, {NULL, 0}, 
    {"WACS_BTEE", WACS_BTEE}, {NULL, 0}, 
    {"WACS_D_RTEE", WACS_D_RTEE}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"WACS_D_BTEE", WACS_D_BTEE}, {NULL, 0}, 
    {"WACS_BOARD", WACS_BOARD}, {NULL, 0}, 
    {"WACS_GEQUAL", WACS_GEQUAL}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"WACS_PI", WACS_PI}, {NULL, 0}, 
    {"WACS_UARROW", WACS_UARROW}, {NULL, 0}, 
    {"WACS_D_URCORNER", WACS_D_URCORNER}, {NULL, 0}, 
    {"WACS_LANTERN", WACS_LANTERN}, {"WACS_URCORNER", WACS_URCORNER},
    {NULL, 0}, {NULL, 0},
    {"WACS_D_LRCORNER", WACS_D_LRCORNER}, {NULL, 0}, 
    {"WACS_D_VLINE", WACS_D_VLINE}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"WACS_DARROW", WACS_DARROW}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"WACS_NEQUAL", WACS_NEQUAL}, {"WACS_TTEE", WACS_TTEE},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"WACS_D_TTEE", WACS_D_TTEE}, {NULL, 0}, 
    {"WACS_LEQUAL", WACS_LEQUAL}, {"WACS_S1", WACS_S1},
    {NULL, 0}, {NULL, 0},
    {"WACS_DIAMOND", WACS_DIAMOND}, {"WACS_S3", WACS_S3},
    {"WACS_D_ULCORNER", WACS_D_ULCORNER}, {NULL, 0}, 
    {"WACS_BULLET", WACS_BULLET}, {"WACS_PLMINUS", WACS_PLMINUS},
    {NULL, 0}, {NULL, 0},
    {"WACS_S7", WACS_S7}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"WACS_HLINE", WACS_HLINE}, {"WACS_S9", WACS_S9},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"WACS_DEGREE", WACS_DEGREE}, {NULL, 0}, 
    {"WACS_LRCORNER", WACS_LRCORNER}, {NULL, 0}, 
    {"WACS_PLUS", WACS_PLUS}, {NULL, 0}, 
    {"WACS_BLOCK", WACS_BLOCK}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"WACS_D_PLUS", WACS_D_PLUS}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"WACS_RARROW", WACS_RARROW}, {NULL, 0}, 
    {"WACS_CKBOARD", WACS_CKBOARD}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
  };
#undef NCOLS
#define NCOLS 2
  /* static int */ WacsMacroNCols = NCOLS;
  /* static int */ WacsMacroNRows = sizeof(tmp) / sizeof(WacsMacro) / NCOLS;
  WacsMacroArray = Jim_Alloc(sizeof(tmp));
  RETURN_IF_ERR(WacsMacroArray == NULL, "%s: Jim_Alloc 1 fail", cmd);
  memcpy(WacsMacroArray, tmp, sizeof(tmp)); 
  /* Form validation macros:
   * Special order required, get details using commands: 
   *     cat wtui.c | ./ckhash code '\{"(TYPE_[A-Z4]+)",' 0.7 1.5 > test1
   *     cat wtui.c | ./ckhash stats '\{"(TYPE_[A-Z4]+)",' 0.7 1.5 */
  FieldTypeMacro ft_tmp[] = {
    {NULL, 0}, {NULL, 0},
    {"TYPE_IPV4", TYPE_IPV4}, {NULL, 0}, 
    {"TYPE_ALNUM", TYPE_ALNUM}, {"TYPE_NUMERIC", TYPE_NUMERIC},
    {"TYPE_INTEGER", TYPE_INTEGER}, {"TYPE_REGEXP", TYPE_REGEXP},
    {"TYPE_ALPHA", TYPE_ALPHA}, {"TYPE_ENUM", TYPE_ENUM},
  };
#undef NCOLS
#define NCOLS 2
  /* static int */ FTMacroNCols = NCOLS;
  /* static int */ FTMacroNRows = sizeof(ft_tmp) / sizeof(FieldTypeMacro) / NCOLS;
  /* static FieldTypeMacro * */ FTMacro = Jim_Alloc(sizeof(ft_tmp));
  RETURN_IF_ERR(FTMacro == NULL, "%s: Jim_Alloc 2 fail", cmd);
  memcpy(FTMacro, ft_tmp, sizeof(ft_tmp));

  if (has_colors() == TRUE) {
    if (start_color() != OK) {
      endwin();
      Jim_SetResultFormatted(interp, "%s: start_color() fail", cmd);
      return JIM_ERR;
    } else {
      /* static int */ ncolors = COLORS;
      /* static int */ ncolorpairs = COLOR_PAIRS;
    }
  }
  raw();
  noecho();
  set_escdelay(escdelay);
  mousemask(ALL_MOUSE_EVENTS|REPORT_MOUSE_POSITION, NULL);
  /* static int */ initialized = 1;

  char name[MAX_CMD_NAME_LEN];
  static int id = 0;
  snprintf(name, sizeof(name), "tui%d", ++id);
  Jim_CreateCommand(interp, name, TuiCmd, NULL, TuiDeleteCmd);
  Jim_SetResultString(interp, name, -1);
  return JIM_OK;
}

int Jim_wtuiInit(Jim_Interp *interp) {
  Jim_CreateCommand(interp, "newtui", TuiCreateCmd, NULL, NULL);
  return JIM_OK;
}

static int wt_colordetail(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  (void)data;
  (void)cmd;
  (void)subcmd;
  if (argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "colordetail");
    return JIM_ERR;
  }
  Jim_Obj *list = Jim_NewListObj(interp, NULL, 0);
  Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp, "ncolors", -1));
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, ncolors));
  Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp, "ncolorpairs", -1));
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, ncolorpairs));
  Jim_SetResult(interp, list);
  return JIM_OK;
}

static NCURSES_COLOR_T getColor(const char *name) {
  /* commands to create array and find nrows and ncols
   *     cat wtui.c | ./ckhash code '\{"(COLOR_[^P][A-Z]+|BRIGHT_[A-Z]+)",' 0.7 1.5 > test1
   *     cat wtui.c | ./ckhash stats '\{"(COLOR_[^P][A-Z]+|BRIGHT_[A-Z]+)",' 0.7 1.5 
   * NOTE: the regexp used above is to avoid including a fragment of the comment
   * command above, as well as to avoid COLOR_PAIRS */
#define BRIGHT(x) (COLOR_ ## x + 8)
  static ColorMacro colors[] = {
    {"COLOR_YELLOW", COLOR_YELLOW}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"COLOR_BLACK", COLOR_BLACK}, {NULL, 0}, 
    {"BRIGHT_MAGENTA", BRIGHT(MAGENTA)}, {"BRIGHT_WHITE", BRIGHT(WHITE)},
    {"COLOR_RED", COLOR_RED}, {NULL, 0}, 
    {"COLOR_BLUE", COLOR_BLUE}, {NULL, 0}, 
    {"COLOR_GREEN", COLOR_GREEN}, {NULL, 0}, 
    {"BRIGHT_CYAN", BRIGHT(CYAN)}, {NULL, 0}, 
    {"BRIGHT_YELLOW", BRIGHT(YELLOW)}, {NULL, 0}, 
    {"BRIGHT_BLACK", BRIGHT(BLACK)}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"BRIGHT_RED", BRIGHT(RED)}, {NULL, 0}, 
    {"COLOR_MAGENTA", COLOR_MAGENTA}, {"COLOR_WHITE", COLOR_WHITE},
    {"BRIGHT_GREEN", BRIGHT(GREEN)}, {NULL, 0}, 
    {"BRIGHT_BLUE", BRIGHT(BLUE)}, {"COLOR_CYAN", COLOR_CYAN},
  };
#undef NCOLS
#define NCOLS 2
  static int ColorMacroNCols = NCOLS;
  static int ColorMacroNRows = sizeof(colors) / sizeof(ColorMacro) / NCOLS;
  return getColorMacroVal(name, colors, ColorMacroNRows, ColorMacroNCols);
}

static int wt_colorpair(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  (void)data;
  if (ncolors == 1) {
    return JIM_OK;
  }
  if (argc < 3 || argc % 3 != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "colorpair COLORPAIRNUM FGCOLORNAME BGCOLORNAME ?... ... ...?");
    return JIM_ERR;
  }
  for (int i = 0; i < argc; i += 3) {
    long pair, colornum;
    RETURN_IF_ERR(Jim_GetLong(interp, argv[i], &pair) != JIM_OK || (pair < 0 || pair > 255), "%s %s: invalid colorpair integer \"%s\"", cmd, subcmd, Jim_String(argv[i]));
    NCURSES_COLOR_T fg, bg;
    if (Jim_GetLong(interp, argv[i+1], &colornum) == JIM_OK) {
      RETURN_IF_ERR(colornum < 0 || colornum > 255, "%s %s: FGCOLORNAME (%s) out of range", cmd, subcmd, Jim_String(argv[i+1]));
      fg = colornum;   
    } else {
      fg = getColor(Jim_String(argv[i+1]));
      RETURN_IF_ERR(fg == (NCURSES_COLOR_T)-1, "%s %s: invalid color name \"%s\"", cmd, subcmd, Jim_String(argv[i+1]));
    }
    if (Jim_GetLong(interp, argv[i+2], &colornum) == JIM_OK) {
      RETURN_IF_ERR(colornum < 0 || colornum > 255, "%s %s: BGCOLORNAME (%s) out of range", cmd, subcmd, Jim_String(argv[i+2]));
      bg = colornum;
    } else {
      bg = getColor(Jim_String(argv[i+2]));
      RETURN_IF_ERR(bg == (NCURSES_COLOR_T)-1, "%s %s: invalid color name \"%s\"", cmd, subcmd, Jim_String(argv[i+2]));
    }
    RETURN_IF_ERR(fg >= ncolors, "%s %s: foreground color number \"%s\" too big", cmd, subcmd, Jim_String(argv[i+1]));
    RETURN_IF_ERR(bg >= ncolors, "%s %s: foreground color number \"%s\" too big", cmd, subcmd, Jim_String(argv[i+2]));
    RETURN_IF_NCERR(init_pair(pair, fg, bg));
  }
  return JIM_OK;
}

static int wt_colwidth(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  (void)data;
  if (argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "colwidth STRING");
    return JIM_ERR;
  }
  const char *string = Jim_String(argv[0]);
  size_t len = mbstowcs(NULL, string, 0); /* size of string in wchars */
  RETURN_IF_ERR(len == (size_t)-1, "%s %s: UTF8 string length unknown \"%s\"", cmd, subcmd, string); 
  wchar_t wbuf[len + 1]; /* + 1 for terminating NUL byte */
  size_t err = mbstowcs(wbuf, string, len + 1);
  RETURN_IF_ERR(err == (size_t)-1, "%s %s: UTF8 string conversion failed \"%s\"", cmd, subcmd, string);
  int width = wcswidth(wbuf, len);
  if (width == -1) {
    Jim_SetResultFormatted(interp, "%s %s: wstring \"%s\" length invalid", cmd, subcmd, string);
    return JIM_ERR;
  } else {
   Jim_SetResultInt(interp, width);
   return JIM_OK;
  }
}

static int wt_cursor(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  (void)data;
  if (argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "cursor invisible|normal|hi-vis");
    return JIM_ERR;
  }
  int type;
  const char *name = Jim_String(argv[0]);
  if (strcmp(name, "invisible") == 0) {
    type = 0;
  } else if (strcmp(name, "normal") == 0) {
    type = 1;
  } else if (strcmp(name, "hi-vis") == 0) {
    type = 2;
  } else {
    Jim_SetResultFormatted(interp, "%s %s: unknown cursor visibility \"%s\"", cmd, subcmd, name);
    return JIM_ERR;	
  }
  RETURN_IF_ERR(curs_set(type) == ERR, "%s %s: TTY does not support cursor \"%s\"", cmd, subcmd, name);
  return JIM_OK;
}

static int z_delete(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  (void)data;
  (void)cmd;
  (void)subcmd;
  if (argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "delete");
    return JIM_ERR;
  }
  Jim_DeleteCommand(interp, argv[-2]); 
  return JIM_OK;
}

static int wt_recolor(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  (void)data;
  if (argc < 4 || argc > 64 || argc % 4 != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "recolor COLORNAME R G B ?... ... ... ...?");
    return JIM_ERR;
  }
  Jim_SetResultInt(interp, 0);
  if (ncolors == 1 || can_change_color() == FALSE) return JIM_OK;
  long red, green, blue;
  for (int i = 0; i < argc; i += 4) {
    const char *name = Jim_String(argv[i]);
    NCURSES_COLOR_T idx = getColor(name);
    RETURN_IF_ERR(idx == (NCURSES_COLOR_T)-1, "%s %s: invalid color name \"%s\"", cmd, subcmd, Jim_String(argv[i]));
    RETURN_IF_ERR(Jim_GetLong(interp, argv[i+1], &red) == JIM_ERR || (red < 0 || red > 1000), "%s %s: R \"%s\" invalid, expected integer in range 0..1000", cmd, subcmd, Jim_String(argv[i+1]));
    RETURN_IF_ERR(Jim_GetLong(interp, argv[i+2], &green) == JIM_ERR || (green < 0 || green > 1000), "%s %s: G \"%s\" invalid, expected integer in range 0..1000", cmd, subcmd, Jim_String(argv[i+2]));
    RETURN_IF_ERR(Jim_GetLong(interp, argv[i+3], &blue) == JIM_ERR || (blue < 0 || blue > 1000), "%s %s: B \"%s\" invalid, expected integer in range 0..1000", cmd, subcmd, Jim_String(argv[i+3]));
    RETURN_IF_ERR(init_color(idx, red, green, blue) == ERR, "%s %s: init_color() fail", cmd, subcmd);
  }
  Jim_SetResultInt(interp, 1);
  return JIM_OK;
}

static int wt_size(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  (void)data;
  (void)cmd;
  (void)subcmd;
  if (argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "size");
    return JIM_ERR;
  }
  int rows, cols;
  getmaxyx(stdscr, rows, cols); /* getmaxyx is a macro: does not take ptrs */
  Jim_Obj *list = Jim_NewListObj(interp, NULL, 0);
  Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp, "rows", -1));
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, rows));
  Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp, "cols", -1));
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, cols));
  Jim_SetResult(interp, list);
  return JIM_OK;
}

static attr_t zgetattr(const char *name) {
  /* Special order required below, by using the following commands to get the
   * array, and nCols and nRows. 
   *     cat wpanel.c | ./ckhash code '\{"(WA_[A-Z]+)",' 0.7 1.5
   *     cat wpanel.c | ./ckhash stats '\{"(WA_[A-Z]+)",' 0.7 1.5
   * See /usr/include/ncurses.h for more info about these macros. */
  static AttrMacro TextAttributes[] = {
    {"WA_NORMAL", WA_NORMAL}, {"WA_TOP", WA_TOP},
    {"WA_HORIZONTAL", WA_HORIZONTAL}, {NULL, 0}, 
    {"WA_BOLD", WA_BOLD}, {NULL, 0}, 
    {"WA_DIM", WA_DIM}, {"WA_UNDERLINE", WA_UNDERLINE},
    {NULL, 0}, {NULL, 0},
    {"WA_LOW", WA_LOW}, {"WA_RIGHT", WA_RIGHT},
    {"WA_ATTRIBUTES", WA_ATTRIBUTES}, {NULL, 0}, 
    {"WA_BLINK", WA_BLINK}, {"WA_REVERSE", WA_REVERSE},
    {"WA_ALTCHARSET", WA_ALTCHARSET}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"WA_INVIS", WA_INVIS}, {"WA_PROTECT", WA_PROTECT},
    {"WA_ITALIC", WA_ITALIC}, {"WA_VERTICAL", WA_VERTICAL},
    {"WA_LEFT", WA_LEFT}, {NULL, 0}, 
    {"WA_STANDOUT", WA_STANDOUT}, {NULL, 0}, 
  };
#undef NCOLS
#define NCOLS 2
  static int TextAttributesNCols = NCOLS;
  static int TextAttributesNRows = sizeof(TextAttributes) / sizeof(AttrMacro) / NCOLS;
  return getAttrMacroVal(name, TextAttributes, TextAttributesNRows, TextAttributesNCols);
}

static int wt_style(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  (void)data;
  if (argc < 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "style COLORPAIR ?ATTR ...?");
    return JIM_ERR;
  }
  long cp;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &cp) != JIM_OK || (cp < 0 || cp > 255), "%s %s: \"%s\" invalid, expected COLORPAIR idx (0 < idx < 256)", cmd, subcmd, Jim_String(argv[0]));
  attr_t attr = COLOR_PAIR((int)cp);
  for (int i = 1; i < argc; ++i) {
    const char *name = Jim_String(argv[i]);
    attr_t single = zgetattr(name);
    RETURN_IF_ERR(single == (attr_t)-1, "%s %s: unknown attribute name \"%s\"", cmd, subcmd, name);
    attr |= single; 
  }
  Jim_SetResultInt(interp, (long)attr);
  return JIM_OK;
}

static int wt_update(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  (void)data;
  (void)cmd;
  (void)subcmd;
  if (argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "update");
    return JIM_ERR;
  }
  update_panels();
  doupdate();
  return JIM_OK;
}

#include "wpanel.c"
#include "wmenu.c"
#include "wform.c"

