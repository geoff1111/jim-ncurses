static int wp_border(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_bottom(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_chgat(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_clear(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_drawline(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_focus(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_getch(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_hidden(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_insert(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_mouse(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_mvcursor(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_position(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_print(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_printalt(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_scroll(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_size(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_style(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_top(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wp_trafo(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);

// See `man Colors` for information about ncurses types
typedef struct {
  PANEL *pnl;
  WINDOW *sub; // actually a derwin 
  int border_active;
} PanelData;

static void PanelDeleteCmd(Jim_Interp *interp, void *privData) {
  (void)interp;
  /* important! free memory in reverse to allocation process */
  PanelData *data = privData;
  if (data) {
    PANEL *pnl = data->pnl;
    WINDOW *win = panel_window(pnl);
    del_panel(pnl);
    if (data->sub) delwin(data->sub);
    if (win) delwin(win);
    Jim_Free(data);
  }
}

static int PanelCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  /* Special order required. Use the following commands to get the data:
   *     cat wpanel.c | ./ckhash code '\{"([a-z]+)",' 0.7 2.5 > test1
   *     cat wpanel.c | ./ckhash stats '\{"([a-z]+)",' 0.7 2.5
   * This latter command allows you to obtain nCols.
   * You have to manually add the wp_ prefix to function names.
   * */
  static SubCmd PanelSubcmds[] = {
    {NULL, 0}, {NULL, 0},
    {"scroll", wp_scroll}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"delete", z_delete}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"hidden", wp_hidden}, {NULL, 0}, 
    {"mvcursor", wp_mvcursor}, {NULL, 0}, 
    {"mouse", wp_mouse}, {NULL, 0}, 
    {"print", wp_print}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"bottom", wp_bottom}, {"drawline", wp_drawline},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"insert", wp_insert}, {NULL, 0}, 
    {"size", wp_size}, {NULL, 0}, 
    {"trafo", wp_trafo}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"border", wp_border}, {NULL, 0}, 
    {"clear", wp_clear}, {"position", wp_position},
    {"getch", wp_getch}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"top", wp_top}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"style", wp_style}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"chgat", wp_chgat}, {NULL, 0}, 
    {"focus", wp_focus}, {NULL, 0}, 
    {"printalt", wp_printalt}, {NULL, 0}, 
  };
#undef NCOLS
#define NCOLS 2
  static int PanelSubcmdsNCols = NCOLS;
  static int PanelSubcmdsNRows = sizeof(PanelSubcmds) / sizeof(SubCmd) / NCOLS;
  if (argc < 2) {
    Jim_WrongNumArgs(interp, 1, argv, "subcommand ?...?");
    return JIM_ERR;
  }
  const char *cmd = Jim_String(argv[0]), *subcmd = Jim_String(argv[1]);
  SubCmdFunc func = getSubCmd(subcmd, PanelSubcmds, PanelSubcmdsNRows, PanelSubcmdsNCols);
  RETURN_IF_ERR(func == NULL, "%s: unknown subcommand \"%s\"", cmd, subcmd);
  RETURN_IF_ERR(! initialized, "%s %s: no current (new) tui (?deleted)", cmd, subcmd);
  return func(Jim_CmdPrivData(interp), cmd, subcmd, interp, argc - 2, argv + 2);
}

static int wp_border(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 0 && argc != 8) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "border BOX ?LEFT RIGHT TOP BOTTOM TL TR BL BR?");
    return JIM_ERR;
  }
  RETURN_IF_ERR(cmd[0] != 'b', "%s %s: this panel does not support borders", cmd, subcmd);
  PanelData *dat = data;
  WINDOW *win = panel_window(dat->pnl);
  if (argc == 0) {
    box(win, 0, 0);
  } else {
    long chars[8];
    cchar_t wc[8];
    for(int i = 0; i < 8; ++i) {
      RETURN_IF_ERR(Jim_GetLong(interp, argv[i], &chars[i]) != JIM_OK, "%s %s: numerical arg %d, \"%s\" not an integer", cmd, subcmd, i + 1, Jim_String(argv[i]));
      wchar_t wchar = (wchar_t)chars[i]; 
      RETURN_IF_ERR(setcchar(wc + i, &wchar, 0, 0, NULL) == ERR, "%s %s: \"%s\" not convertible to cchar", cmd, subcmd, Jim_String(argv[i]));
    }
    RETURN_IF_ERR(wborder_set(win, wc, wc + 1, wc + 2, wc + 3, wc + 4, wc + 5, wc + 6, wc + 7) == ERR, "%s %s: border could not be set", cmd, subcmd);
  }
  return JIM_OK;
}

static int wp_bottom(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv  - 2, "bottom");
    return JIM_ERR;
  }
  (void)cmd;
  (void)subcmd;
  PanelData *dat = data;
  PANEL *pnl = dat->pnl;
  bottom_panel(pnl);
  return JIM_OK;
}

static int wp_chgat(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc ,Jim_Obj * const *argv) {
  if (argc != 2) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "chgat NCHARS STYLE");
    return JIM_ERR;
  }
  PanelData *dat = data;
  WINDOW *active = cmd[0] != 'b' || dat->border_active ? panel_window(dat->pnl) : dat->sub;
  long n, style; 
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &n) != JIM_OK, "%s %s: invalid nchars (int): %s", cmd, subcmd, Jim_String(argv[0]));
  RETURN_IF_ERR(Jim_GetLong(interp, argv[1], &style) != JIM_OK, "%s %s: invalid style: %s", cmd, subcmd, Jim_String(argv[1]));
  RETURN_IF_ERR(wchgat(active, n, (attr_t)style, PAIR_NUMBER((chtype)style), NULL) != OK, "%s %s: unable to change panel attributes/colorpair", cmd, subcmd);
  return JIM_OK;
}

static int wp_clear(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "clear screen|right|below|line|char");
    return JIM_ERR;
  }
  PanelData *dat = data;
  WINDOW *active = cmd[0] == 'b' && ! dat->border_active ? dat->sub : panel_window(dat->pnl);
  const char *what = Jim_String(argv[0]);
  if (strcmp(what, "right") == 0) {
    wclrtoeol(active);
  } else if (strcmp(what, "below") == 0) {
    wclrtobot(active);
  } else if (strcmp(what, "line") == 0) {
    wdeleteln(active);
  } else if (strcmp(what, "char") == 0) {
    wdelch(active);
  } else if (strcmp(what, "screen") == 0) {
    werase(active);
  } else {
    Jim_SetResultFormatted(interp, "%s %s: \"%s\" unknown", cmd, subcmd, what);
    return JIM_ERR;
  }
  return JIM_OK;
}

static int wp_drawline(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 2 && argc != 3) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "drawline right|down N ?CODE_PT|WACS_MACRO?");
    return JIM_ERR;
  }
  const char *direction = Jim_String(argv[0]);
  long n, ch;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[1], &n) != JIM_OK, "%s %s: N \"%s\" not an integer", cmd, subcmd, Jim_String(argv[1]));
  cchar_t *wc = NULL, wcvalue;
  if (argc == 3) {
    if (Jim_GetLong(interp, argv[2], &ch) == JIM_OK) {
      wchar_t wchar = (wchar_t)ch;
      RETURN_IF_ERR(setcchar(&wcvalue, &wchar, 0, 0, NULL) == ERR, "%s %s: \"%s\" not convertible to cchar", cmd, subcmd, Jim_String(argv[2]));
      wc = &wcvalue;
    } else {
      const char *name = Jim_String(argv[2]);
      wc = getWacsMacroVal(name, WacsMacroArray, WacsMacroNRows, WacsMacroNCols);
      RETURN_IF_ERR(wc == NULL, "%s %s: unknown alt char: \"%s\" (or supply wch integer)", cmd, subcmd, name);
    }
  }
  PanelData *dat = data;
  WINDOW *active = cmd[0] == 'b' && ! dat->border_active ? dat->sub : panel_window(dat->pnl);
  if (strcmp(direction, "right") == 0) {
    if (argc == 2) { // need a default horizontal cchar_t
      wc = getWacsMacroVal("WACS_HLINE", WacsMacroArray, WacsMacroNRows, WacsMacroNCols);
    }
    whline_set(active, wc, n);
  } else if (strcmp(direction, "down") == 0) {
    if (argc == 2) { // need a default vertical cchar_t
      wc = getWacsMacroVal("WACS_VLINE", WacsMacroArray, WacsMacroNRows, WacsMacroNCols);
    }
    wvline_set(active, wc, n);
  } else {
    Jim_SetResultFormatted(interp, "%s %s: \"%s\" invalid, should be right|down", cmd, subcmd, direction);
    return JIM_ERR;
  }
  return JIM_OK;
}

/* returns <keycode=0|1> <codepoint> <keyname>. In ndelay (non-blocking) can return more than one triple if there is extra chars to process.
   in non-blocking mode, returns {0 0 ""} if would block */
static int wp_getch(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "getch");
    return JIM_ERR;
  }
  PanelData *dat = data;
  WINDOW *active = cmd[0] == 'b' && ! dat->border_active ? dat->sub : panel_window(dat->pnl);
  Jim_Obj *list = Jim_NewListObj(interp, NULL, 0);
  wint_t wch;
  for (int type;;) {
    type = wget_wch(active, &wch);
    if (type == ERR) {
      if (noblock) {
        Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, 0));
        Jim_ListAppendElement(interp, list, Jim_NewWideObj(interp, 0));
        Jim_ListAppendElement(interp, list, Jim_NewEmptyStringObj(interp));
	break;
      } else {
        RETURN_IF_ERR(type == ERR, "%s %s: invalid char", cmd, subcmd);
      }
    } else {
      Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, type == KEY_CODE_YES));
      Jim_ListAppendElement(interp, list, Jim_NewWideObj(interp, (jim_wide)wch));
      Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp, keyname(wch), -1));
      if (! noblock) break; // blocking, do not loop, otherwise need to drain all input
    }
  }
  Jim_SetResult(interp, list);
  return JIM_OK;
}

static int wp_insert(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "insert line|CODE_PT");
    return JIM_ERR;
  }
  const char *what = Jim_String(argv[0]);
  PanelData *dat = data;
  WINDOW *active = cmd[0] == 'b' && ! dat->border_active ? dat->sub : panel_window(dat->pnl);
  if (strcmp(what, "line") == 0) {
    winsertln(active);
  } else {
    long ch;
    RETURN_IF_ERR(Jim_GetLong(interp, argv[1], &ch) != JIM_OK, "%s %s: \"%s\" not an integer", cmd, subcmd, Jim_String(argv[0]));
    wchar_t wchar = (wchar_t)ch; 
    cchar_t wc;
    RETURN_IF_ERR(setcchar(&wc, &wchar, 0, 0, NULL) == ERR, "%s %s: \"%s\" not convertible to cchar", cmd, subcmd, Jim_String(argv[0]));
    wins_wch(active, &wc);
  }
  return JIM_OK;
}

static int wp_focus(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 0 && argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "focus border|inside");
    return JIM_ERR;
  }
  PanelData *dat = data;
  if (argc == 1) {
    const char *focus = Jim_String(argv[0]);
    if (strcmp(focus, "border") == 0) dat->border_active = 1;
    else if (strcmp(focus, "inside") == 0) dat->border_active = 0;
    else {
      Jim_SetResultFormatted(interp, "%s %s: unknown focus \"%s\", expected border|inside", cmd, subcmd, focus);
      return JIM_ERR;
    }
  }
  Jim_SetResultString(interp, dat->border_active ? "border" : "inside", -1);
  return JIM_OK;
}

static int wp_hidden(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 0 && argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "hide ?yes|no?");
    return JIM_ERR;
  }
  PanelData *dat = data;
  PANEL *pnl = dat->pnl;
  if (argc == 1) {
    const char *op = Jim_String(argv[0]);
    if (strcmp(op, "yes") == 0) {
      hide_panel(pnl);
    } else if (strcmp(op, "no") == 0) {
      show_panel(pnl);
    } else {
      Jim_SetResultFormatted(interp, "%s %s: \"%s\" unknown, should be \"yes\" or \"no\"", cmd, 
	subcmd, op);
      return JIM_ERR;
    }
  }
  Jim_SetResultInt(interp, panel_hidden(pnl));
  return JIM_OK;
}

static int wp_mouse(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "mouse");
    return JIM_ERR;
  }
  (void)cmd;
  (void)subcmd;
  PanelData *dat = data;
  WINDOW *win = panel_window(dat->pnl);
  WINDOW *sub = dat->sub;
  Jim_Obj *list = Jim_NewListObj(interp, NULL, 0);
  MEVENT mevent;
  getmouse(&mevent);
  Jim_ListAppendElement(interp,list,Jim_NewStringObj(interp,"event",-1));
  int i;
  for (i = 0; i < 4; ++i) {
    if (BUTTON_RELEASE(mevent.bstate, i)) {
      Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp, "release", -1));
      break;
    } else if (BUTTON_PRESS(mevent.bstate, i)) {
      Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp,"press",-1));
      break;
    } else if (BUTTON_CLICK(mevent.bstate, i)) {
      Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp,"click",-1));
      break;
    } else if (BUTTON_DOUBLE_CLICK(mevent.bstate, i)) {
      Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp, "dclick", -1));
      break;
    } else if (BUTTON_TRIPLE_CLICK(mevent.bstate, i)) {
      Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp, "tclick", -1));
      break;
    }
  }
  if (i == 4) {
    Jim_ListAppendElement(interp,list,Jim_NewEmptyStringObj(interp));
  } else { 
    Jim_ListAppendElement(interp,list,Jim_NewStringObj(interp, "button", -1));
    Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, i));
  }
  Jim_ListAppendElement(interp,list,Jim_NewStringObj(interp, "modifier", -1));
  if (mevent.bstate & BUTTON_CTRL) {
    Jim_ListAppendElement(interp,list,Jim_NewStringObj(interp, "ctrl", -1));
  } else if (mevent.bstate & BUTTON_SHIFT) {
    Jim_ListAppendElement(interp,list,Jim_NewStringObj(interp, "shift", -1));
  } else if (mevent.bstate & BUTTON_ALT) {
    Jim_ListAppendElement(interp,list,Jim_NewStringObj(interp, "alt", -1));
  } else {
    Jim_ListAppendElement(interp,list,Jim_NewEmptyStringObj(interp));
  }
  int begy, begx, ht, wd, oht, owd, oy = -1, ox = -1;
  if (sub != NULL) {
    getbegyx(sub, begy, begx);
    getmaxyx(sub, ht, wd);
    int obegy, obegx;
    getbegyx(win, obegy, obegx);
    getmaxyx(win, oht, owd);
    oy = mevent.y - obegy;
    ox = mevent.x - obegx;
  } else {
    getbegyx(win, begy, begx);
    getmaxyx(win, ht, wd);
  }
  int winy = mevent.y - begy;
  int winx = mevent.x - begx;
  int row, col;
  const char *inside;
  if (winy >= 0 && winy < ht && winx >= 0 && winx < wd) {
    inside = "inner";
    row = winy;
    col = winx;
  } else if (sub != NULL && oy >= 0 && oy < oht && ox >= 0 && ox < owd) {
    inside = "border";
    row = oy;
    col = ox;    
  } else {
    inside = "outer";
    row = mevent.y;
    col = mevent.x;
  }
  Jim_ListAppendElement(interp,list,Jim_NewStringObj(interp, "loc", -1));
  Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp, inside, -1));
  Jim_ListAppendElement(interp,list,Jim_NewStringObj(interp, "row", -1));
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, row));
  Jim_ListAppendElement(interp,list,Jim_NewStringObj(interp, "col",-1));
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, col));
  Jim_SetResult(interp, list);
  return JIM_OK;
}

static int wp_mvcursor(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 2 && argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "mvcursor ?ROW COL?");
    return JIM_ERR;
  }
  PanelData *dat = data;
  WINDOW *active = cmd[0] == 'b' && ! dat->border_active ? dat->sub : panel_window(dat->pnl);
  int nrows, ncols;
  getmaxyx(active, nrows, ncols);
  if (argc == 2) {
    long y, x;
    RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &y) != JIM_OK, "%s %s: invalid row coordinate %s", cmd, subcmd, Jim_String(argv[0]));
    RETURN_IF_ERR(Jim_GetLong(interp, argv[1], &x) != JIM_OK, "%s %s: invalid col coordinate %s", cmd, subcmd, Jim_String(argv[1]));
    RETURN_IF_ERR(y < 0 || y >= nrows || x < 0 || x >= ncols, "%s %s: coordinates %s %s out of bounds", cmd, subcmd, Jim_String(argv[0]), Jim_String(argv[1]));
    wmove(active, y, x);
  }
  int row, col;
  getyx(active, row, col);
  Jim_Obj *list = Jim_NewListObj(interp, NULL, 0);
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, row));
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, col));
  Jim_SetResult(interp, list);
  return JIM_OK;
}

static int wp_newpanel(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  (void)data;
  if (argc != 4 && argc != 8) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "newpanel HEIGHT WIDTH YBEGIN XBEGIN ?TOPMARGIN BOTTOM LEFT RIGHT?");
    return JIM_ERR;
  }
  long height, width, begin_y, begin_x, by = 0, bottom = 0, bx = 0, right = 0, ht = 0, wd = 0;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &height) != JIM_OK, "%s %s: invalid height: %s", cmd, subcmd, Jim_String(argv[0]));
  RETURN_IF_ERR(Jim_GetLong(interp, argv[1], &width) != JIM_OK, "%s %s: invalid width: %s", cmd, subcmd, Jim_String(argv[1]));
  RETURN_IF_ERR(Jim_GetLong(interp, argv[2], &begin_y) != JIM_OK, "%s %s: invalid begin_y: %s", cmd, subcmd, Jim_String(argv[2]));
  RETURN_IF_ERR(Jim_GetLong(interp, argv[3], &begin_x) != JIM_OK, "%s %s: invalid begin_x: %s", cmd, subcmd, Jim_String(argv[3]));
  PanelData *new = Jim_Alloc(sizeof(PanelData));
  new->border_active = 0;
  new->sub = NULL;
  RETURN_IF_ERR(!new, "%s: failed to alloc PanelData", cmd);
  WINDOW *win = newwin(height, width, begin_y, begin_x);
  RETURN_IF_ERR(!win, "%s: failed to create win", cmd);
  keypad(win, TRUE);
  nodelay(win, noblock);// static bool set in tui.create
  const char *procname = "panel%d";
  if (argc == 8) {
    RETURN_IF_ERR(Jim_GetLong(interp, argv[4], &by) != JIM_OK, "%s %s: invalid top margin: %s", cmd, subcmd, Jim_String(argv[4]));
    RETURN_IF_ERR(Jim_GetLong(interp, argv[5], &bottom) != JIM_OK, "%s %s: invalid bottom margin: %s", cmd, subcmd, Jim_String(argv[5]));
    RETURN_IF_ERR(Jim_GetLong(interp, argv[6], &bx) != JIM_OK, "%s %s: invalid left margin: %s", cmd, subcmd, Jim_String(argv[6]));
    RETURN_IF_ERR(Jim_GetLong(interp, argv[7], &right) != JIM_OK, "%s %s: invalid right margin: %s", cmd, subcmd, Jim_String(argv[7]));
    ht = height - by - bottom;
    wd = width - bx - right;
    RETURN_IF_ERR(by < 0 || bx < 0 || ht < 2 || wd < 2, "%s %s: margins (%d %d %d %d) make derived window < 2x2", cmd, subcmd, (int)by, (int)bottom, (int)bx, (int)right);
    WINDOW *sub = derwin(win, ht, wd, by, bx);
    RETURN_IF_ERR(! sub, "%s: failed to create derwin", cmd);
    keypad(sub, TRUE);
    nodelay(sub, noblock);// static bool set in tui.create
    scrollok(sub, TRUE);
    syncok(sub, TRUE); // IMPORTANT: this updates win and sub when sub is changed otherwise 1 keystroke behind in ndelay mode
    new->sub = sub;
    procname = "bpanel%d";
  } else {
    scrollok(win, TRUE);
  }
  new->pnl = new_panel(win);
  RETURN_IF_ERR(!new->pnl, "%s %s: failed to create panel", cmd, subcmd);
  char name[MAX_CMD_NAME_LEN];
  static int id = 0;
  snprintf(name, sizeof(name), procname, ++id);
  Jim_CreateCommand(interp, name, PanelCmd, new, PanelDeleteCmd);
  Jim_SetResultString(interp, name, -1);
  return JIM_OK;
}

static int wp_position(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 0 && argc != 2) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "position ?ROW COL?");
    return JIM_ERR;
  }
  PanelData *dat = data;
  PANEL *pnl = dat->pnl;
  if (argc == 2) {
    long y, x;
    RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &y) != JIM_OK, "%s %s: invalid row: %s", cmd, subcmd, Jim_String(argv[0]));
    RETURN_IF_ERR(Jim_GetLong(interp, argv[1], &x) != JIM_OK, "%s %s: invalid col: %s", cmd, subcmd, Jim_String(argv[1]));
    move_panel(pnl, (int)y, (int)x);
  }
  WINDOW *win = panel_window(pnl);
  int row, col;
  getbegyx(win, row, col);
  Jim_Obj *list = Jim_NewListObj(interp, NULL, 0);
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, row));
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, col));
  Jim_SetResult(interp, list);
  return JIM_OK;
}  

static int wp_print(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "print STRING");
    return JIM_ERR;
  }
  const char *string = Jim_String(argv[0]);
  size_t len = mbstowcs(NULL, string, 0); /* size of string in wchars */
  RETURN_IF_ERR(len == (size_t)-1, "%s %s: UTF8 string length unknown \"%s\"", cmd, subcmd, string); 
  wchar_t wbuf[len + 1]; /* + 1 for terminating NUL byte */
  size_t err = mbstowcs(wbuf, string, len+1);
  RETURN_IF_ERR(err == (size_t)-1, "%s %s: UTF8 string conversion failed \"%s\"", cmd, subcmd, string); 
  PanelData *dat = data;
  WINDOW *active = cmd[0] == 'b' && ! dat->border_active ? dat->sub : panel_window(dat->pnl);
  RETURN_IF_ERR(waddwstr(active, wbuf) == ERR, "%s %s: UTF8 string print failed \"%s\"", cmd, subcmd, string); 
  return JIM_OK;
}

static int wp_printalt(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "print ALTCHARNAME");
    return JIM_ERR;
  }
  const char *name = Jim_String(argv[0]);
  cchar_t *ch = getWacsMacroVal(name, WacsMacroArray, WacsMacroNRows, WacsMacroNCols);
  RETURN_IF_ERR(ch == NULL, "%s %s: unknown alt char: \"%s\"", cmd, subcmd, name);
  PanelData *dat = data;
  WINDOW *active = cmd[0] == 'b' && ! dat->border_active ? dat->sub : panel_window(dat->pnl);
  RETURN_IF_ERR(wadd_wch(active, ch) != OK, "%s %s: unable to add alt char \"%s\"", cmd, subcmd, name); 
  return JIM_OK;
}

static int wp_scroll(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "scroll N");
    return JIM_ERR;
  }
  long n;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &n) != JIM_OK, "%s %s: \"%s\" not an integer", cmd, subcmd, Jim_String(argv[0]));
  PanelData *dat = data;
  WINDOW *mainwin = cmd[0] == 'b' ? dat->sub : panel_window(dat->pnl);
  RETURN_IF_NCERR(wscrl(mainwin, n));
  return JIM_OK;
}

static int wp_size(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 0 && argc != 2) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "size ?ROWS COLS?");
    return JIM_ERR;
  }
  PanelData *dat = data;
  PANEL *pnl = dat->pnl;
  WINDOW *win = panel_window(pnl);
  WINDOW *sub = dat->sub;
  if (argc == 2) {
    long height, width;
    RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &height) != JIM_OK, "%s %s: invalid row dimension %s", cmd, subcmd, Jim_String(argv[0]));
    RETURN_IF_ERR(Jim_GetLong(interp, argv[1], &width) != JIM_OK, "%s %s: invalid col dimension %s", cmd, subcmd, Jim_String(argv[1]));
    int oh, ow;
    getmaxyx(win, oh, ow); // original outer size is needed to calculate borders before resizing
    RETURN_IF_ERR(wresize(win, height, width) != OK, "%s %s: window resize fail %d %d", cmd, subcmd, height, width);
    if (sub) {
      int new_ih, new_iw, ih, iw; // offy, offx;
      getmaxyx(sub, ih, iw);
      new_ih = height - oh - ih;
      new_iw = width - ow + iw;
      RETURN_IF_ERR(new_ih <= 0, "%s %s: new interior height %d < 0", cmd, subcmd, new_ih); 
      RETURN_IF_ERR(new_iw <= 0, "%s %s: new interior width %d < 0", cmd, subcmd, new_iw); 
      RETURN_IF_ERR(wresize(sub, new_ih, new_iw) != OK, "%s %s: interior window resize fail %d %d", cmd, subcmd, height, width);
    }
    RETURN_IF_ERR(replace_panel(pnl, win) != OK, "%s %s: panel replace fail", cmd, subcmd);
  }
  int rows, cols;
  getmaxyx(win, rows, cols);
  Jim_Obj *list = Jim_NewListObj(interp, NULL, 0);
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, rows));
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, cols));
  Jim_SetResult(interp, list);
  return JIM_OK;
}

static int wp_style(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "style STYLE");
    return JIM_ERR;
  }
  long style;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &style) != JIM_OK, "%s %s: expected integer, got \"%s\"", cmd, subcmd, Jim_String(argv[0]));
  PanelData *dat = data;
  WINDOW *active = cmd[0] == 'b' && ! dat->border_active ? dat->sub : panel_window(dat->pnl);
  RETURN_IF_ERR(wattrset(active, (attr_t)style) != OK, "%s %s: unable to enable style \"%s\"", cmd, subcmd, Jim_String(argv[0]));
  return JIM_OK;
}

static int wp_top(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "top");
    return JIM_ERR;
  }
  (void)cmd;
  (void)subcmd;
  PanelData *dat = data;
  PANEL *pnl = dat->pnl;
  top_panel(pnl);
  return JIM_OK;
}

static int wp_trafo(void *data, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc ,Jim_Obj * const *argv) {
  /* NOTE: this function must not be called on an inside (panel relative) 
   * coordinate, it must be called on a screen relative coordinate (not inside)
   * (wmouse_trafo does not give the result i expect (if mouse click is above
   * or to left of bottom right corner of window (even outside of window) it
   * adds begy and begx to the coordinates respectively)). */
  if (argc != 2) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "trafo ROW COL");
    return JIM_ERR;
  }
  long y, x; 
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &y) != JIM_OK, "%s %s: invalid row: %s", cmd, subcmd, Jim_String(argv[0]));
  RETURN_IF_ERR(Jim_GetLong(interp, argv[1], &x) != JIM_OK, "%s %s: invalid col: %s", cmd, subcmd, Jim_String(argv[1]));
  Jim_Obj *list = Jim_NewListObj(interp, NULL, 0);
  PanelData *dat = data;
  WINDOW *win = panel_window(dat->pnl);
  WINDOW *sub = dat->sub;
  int begy, begx, ht, wd, oht, owd, oy = -1, ox = -1;
  if (sub != NULL) {
    getbegyx(sub, begy, begx);
    getmaxyx(sub, ht, wd);
    int obegy, obegx;
    getbegyx(win, obegy, obegx);
    getmaxyx(win, oht, owd);
    oy = y - obegy;
    ox = x - obegx;
  } else {
    getbegyx(win, begy, begx);
    getmaxyx(win, ht, wd);
  }
  int winy = y - begy;
  int winx = x - begx;
  int row, col;
  const char *inside;
  if (winy >= 0 && winy < ht && winx >= 0 && winx < wd) {
    inside = "inner";
    row = winy;
    col = winx;
  } else if (sub != NULL && oy >= 0 && oy < oht && ox >= 0 && ox < owd) {
    inside = "border";
    row = oy;
    col = ox;    
  } else {
    inside = "outer";
    row = y;
    col = x;
  }
  Jim_ListAppendElement(interp,list,Jim_NewStringObj(interp, "loc", -1));
  Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp, inside, -1));
  Jim_ListAppendElement(interp,list,Jim_NewStringObj(interp, "row", -1));
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, row));
  Jim_ListAppendElement(interp,list,Jim_NewStringObj(interp, "col",-1));
  Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, col));
  Jim_SetResult(interp, list);
  return JIM_OK;
}
