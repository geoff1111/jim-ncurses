static int wm_driver(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wm_format(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wm_getchoice(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wm_itemdetail(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wm_mark(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wm_newpanel(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wm_option(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wm_post(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wm_unpost(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);

static void MenuDeleteCmd(Jim_Interp *interp, void *privData) {
  (void)interp;
  /* free memory in reverse to allocation process */
  if (privData) {
    ITEM **list = menu_items(privData);
    free_menu(privData);
    for (int i = 0; list[i] != NULL; ++i) {
      void *name = (void *)item_name(list[i]);
      void *desc = (void *)item_description(list[i]);
      free_item(list[i]);      
      if (name) Jim_Free(name);
      if (desc) Jim_Free(desc);
    }
    Jim_Free(list);
  }
}

static int MenuCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  /* Special order required, get details using commands: 
   *     cat wmenu.c | ./ckhash code '\{"([a-z]+)",' 0.7 1.5 > test1
   *     cat wmenu.c | ./ckhash stats '\{"([a-z]+)",' 0.7 1.5 
   * Then wm_ or z_ needs to be manually added to the function names */
  static SubCmd MenuSubcmds[] = {
    {"driver", wm_driver}, {"post", wm_post},
    {"unpost", wm_unpost}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"delete", z_delete}, {NULL, 0}, 
    {"itemdetail", wm_itemdetail}, {"newpanel", wm_newpanel},
    {"mark", wm_mark}, {"option", wm_option},
    {NULL, 0}, {NULL, 0},
    {"format", wm_format}, {"getchoice", wm_getchoice},
  };
#undef NCOLS
#define NCOLS 2
  static int MenuSubcmdsNCols = NCOLS;
  static int MenuSubcmdsNRows = sizeof(MenuSubcmds) / sizeof(SubCmd) / NCOLS;
  if (argc < 2) {
    Jim_WrongNumArgs(interp, 1, argv, "subcommand ?...?");
    return JIM_ERR;
  }
  const char *cmd = Jim_String(argv[0]);
  const char *subcmd = Jim_String(argv[1]);
  SubCmdFunc func = getSubCmd(subcmd, MenuSubcmds, MenuSubcmdsNRows, MenuSubcmdsNCols);
  RETURN_IF_ERR(!func, "%s: unknown subcommand \"%s\"", cmd, subcmd);
  RETURN_IF_ERR(! initialized, "%s %s: no current (new) tui (?deleted)", cmd, subcmd);
  return func(Jim_CmdPrivData(interp), cmd, subcmd, interp, argc - 2, argv + 2);
}

static int wm_newmenu(void *menu, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  (void)menu;
  if (argc < 2 || argc % 2 != 0) {
    Jim_WrongNumArgs(interp, 1, argv, "newmenu ITEM_NAME ITEM_DESCR ?... ...?");
    return JIM_ERR;
  }
  const char *item, *desc;
  ITEM **itemlist = Jim_Alloc((argc / 2 + 1) * sizeof(ITEM*));
  RETURN_IF_ERR(! itemlist, "%s %s: menu create alloc fail 1", cmd, subcmd);
  int j = 0;
  for (int i = 0; i < argc; i += 2, ++j) {
    item = Jim_String(argv[i]);
    desc = Jim_String(argv[i + 1]);
    itemlist[j] = new_item(Jim_StrDup(item), *desc != '\0' ? Jim_StrDup(desc) : NULL);
    RETURN_IF_ERR(item_name(itemlist[j]) == NULL, "%s %s: menu create alloc fail 2", cmd, subcmd);
  }
  itemlist[j] = NULL;
  MENU *privData = new_menu(itemlist);
  static int id = 0;
  char name[MAX_CMD_NAME_LEN];
  snprintf(name, sizeof(name), "menu%d", ++id);
  Jim_CreateCommand(interp, name, MenuCmd, privData, MenuDeleteCmd);
  Jim_SetResultString(interp, name, -1);
  return JIM_OK;
}

static int wm_newpanel(void *menu, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 2 && argc != 6) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "newpanel YBEGIN XBEGIN ?TOP_MARGIN BOTTOM LEFT RIGHT?");
    return JIM_ERR;
  }
  long begy, begx;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &begy) != JIM_OK, "%s %s: invalid begin_y integer %s", cmd, subcmd, Jim_String(argv[0]));
  RETURN_IF_ERR(Jim_GetLong(interp, argv[1], &begx) != JIM_OK, "%s %s: invalid begin_x integer %s", cmd, subcmd, Jim_String(argv[1]));
  int oh, ow, ih, iw;
  int status = scale_menu(menu, &ih, &iw);
  RETURN_IF_ERR(status != OK, "%s %s: scale_menu fail: %d", cmd, subcmd, status);
  long top, bottom, left, right;
  if (argc == 6) {
    RETURN_IF_ERR(Jim_GetLong(interp, argv[2], &top) != JIM_OK, "%s %s: invalid top_margin integer %s", cmd, subcmd, Jim_String(argv[2]));
    RETURN_IF_ERR(Jim_GetLong(interp, argv[3], &bottom) != JIM_OK, "%s %s: invalid bottom_margin integer %s", cmd, subcmd, Jim_String(argv[3]));
    RETURN_IF_ERR(Jim_GetLong(interp, argv[4], &left) != JIM_OK, "%s %s: invalid left_margin integer %s", cmd, subcmd, Jim_String(argv[4]));
    RETURN_IF_ERR(Jim_GetLong(interp, argv[5], &right) != JIM_OK, "%s %s: invalid right_margin integer %s", cmd, subcmd, Jim_String(argv[5]));
    oh = ih + top + bottom;
    ow = iw + left + right;
  } else {
    oh = ih;
    ow = iw;
  }
  int maxy, maxx;
  getmaxyx(stdscr, maxy, maxx);
  RETURN_IF_ERR(oh + begy >= maxy || ow + begx >= maxx, "%s %s: panel extent (%d, %d) exceeds scr size (%d, %d)", cmd, subcmd, oh + begy, ow + begx, maxy, maxx);

  PanelData *new = Jim_Alloc(sizeof(PanelData));
  RETURN_IF_ERR(!new, "%s %s: failed to alloc PanelData", cmd, subcmd);
  new->border_active = 0;
  new->sub = NULL;
  WINDOW *win = newwin(oh, ow, begy, begx);
  RETURN_IF_ERR(!win, "%s %s: failed to create win", cmd, subcmd);
  keypad(win, TRUE);
  nodelay(win, noblock);// static bool set in tui.create
  PANEL *pnl = new_panel(win);
  RETURN_IF_ERR(!pnl, "%s %s: new_panel fail", cmd, subcmd);
  new->pnl = pnl;
  RETURN_IF_ERR(set_menu_win(menu, win) != OK, "%s %s: set_menu_win fail", cmd, subcmd);
  const char *procname = "panel%d";
  if (argc == 6) {
    WINDOW *sub = derwin(win, ih, iw, top, left);
    RETURN_IF_ERR(! sub, "%s %s: derwin fail", cmd, subcmd);
    new->sub = sub;
    keypad(sub, TRUE);
    nodelay(win, noblock);// static bool set in tui.create
    RETURN_IF_ERR(set_menu_sub(menu, sub) != OK, "%s %s: set_menu_sub fail", cmd, subcmd); 
    procname = "bpanel%d";
  }
  char name[MAX_CMD_NAME_LEN];
  static int id = 0;
  snprintf(name, MAX_CMD_NAME_LEN, procname, ++id);
  Jim_CreateCommand(interp, name, PanelCmd, new, PanelDeleteCmd);
  Jim_SetResultString(interp, name, -1);
  return JIM_OK;
}

static int wm_driver(void *menu, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  /* Special order required, get details using commands: 
   *     cat wmenu.c | ./ckhash code '\{"(REQ_[A-Z_]+|KEY_MOUSE)",' 0.7 1.5 > test1
   *     cat wmenu.c | ./ckhash stats '\{"(REQ_[A-Z_]+|KEY_MOUSE)",' 0.7 1.5 */
  static IntMacro MenuAction[] = {
    {NULL, 0}, {NULL, 0},
    {"REQ_BACK_PATTERN", REQ_BACK_PATTERN}, {"REQ_DOWN_ITEM", REQ_DOWN_ITEM},
    {"REQ_LEFT_ITEM", REQ_LEFT_ITEM}, {NULL, 0}, 
    {"REQ_SCR_ULINE", REQ_SCR_ULINE}, {NULL, 0}, 
    {"REQ_NEXT_MATCH", REQ_NEXT_MATCH}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"REQ_SCR_DLINE", REQ_SCR_DLINE}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"REQ_SCR_UPAGE", REQ_SCR_UPAGE}, {"REQ_UP_ITEM", REQ_UP_ITEM},
    {"REQ_CLEAR_PATTERN", REQ_CLEAR_PATTERN}, {NULL, 0}, 
    {"REQ_PREV_MATCH", REQ_PREV_MATCH}, {NULL, 0}, 
    {"REQ_SCR_DPAGE", REQ_SCR_DPAGE}, {"REQ_TOGGLE_ITEM", REQ_TOGGLE_ITEM},
    {NULL, 0}, {NULL, 0},
    {"REQ_RIGHT_ITEM", REQ_RIGHT_ITEM}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"KEY_MOUSE", KEY_MOUSE}, {NULL, 0}, 
    {"REQ_PREV_ITEM", REQ_PREV_ITEM}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"REQ_NEXT_ITEM", REQ_NEXT_ITEM}, {NULL, 0}, 
    {"REQ_FIRST_ITEM", REQ_FIRST_ITEM}, {"REQ_LAST_ITEM", REQ_LAST_ITEM},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
  };
#undef NCOLS
#define NCOLS 2
  static int MenuActionNRows = sizeof(MenuAction) / sizeof(IntMacro) / NCOLS;
  static int MenuActionNCols = NCOLS;
  if (argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "driver CODEPOINT|MENU-ACTION");
    return JIM_ERR;
  }
  const char *action = Jim_String(argv[0]);
  if (*action == 'R' || *action == 'K') {
    int macro = getIntMacroVal(action, MenuAction, MenuActionNRows, MenuActionNCols);
    RETURN_IF_ERR(macro == -1, "%s %s: unknown menu action \"%s\"", cmd, subcmd, action) 
    RETURN_IF_NCERR(menu_driver(menu, macro));
  } else {
    long ch;
    RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &ch) != JIM_OK, "%s %s: codepoint invalid integer \"%s\"", cmd, subcmd, action);
    RETURN_IF_ERR(ch < 1 || ch > 127, "%s %s: codepoint out of range \"%s\"", cmd, subcmd, action);
    RETURN_IF_NCERR(menu_driver(menu, (int)ch));
  }
  return JIM_OK;
}

static int wm_getchoice(void *menu, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  (void)cmd;
  (void)subcmd;
  if (argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "getchoice");
    return JIM_ERR;
  }
  ITEM **items = menu_items(menu);
  int nitem = item_count(menu);
  Jim_Obj *list = Jim_NewListObj(interp, NULL, 0);
  if (menu_opts(menu) & O_ONEVALUE) { /* single selection */
    ITEM *selected = current_item(menu);
    for (int i = 0; i < nitem; ++i) {
      if (items[i] == selected) {
	Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, i));
	break;
      }
    }
  } else { /* multi-select allowed */
    for (int i = 0; i < nitem; ++i) {
      if (item_value(items[i]) == TRUE) {
        Jim_ListAppendElement(interp, list, Jim_NewIntObj(interp, i));
      }
    }
  }
  Jim_SetResult(interp, list);
  return JIM_OK;
}

static int wm_format(void *menu, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 2) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "format ROWS COLS");
    return JIM_ERR;
  }
  long rows, cols;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &rows) != JIM_OK, "%s %s: invalid row dimension: %s", cmd, subcmd, Jim_String(argv[0]));
  RETURN_IF_ERR(Jim_GetLong(interp, argv[1], &cols) != JIM_OK, "%s %s: invalid col dimension: %s", cmd, subcmd, Jim_String(argv[1]));
  RETURN_IF_NCERR(set_menu_format(menu, rows, cols));
  return JIM_OK;    
}

static int wm_itemdetail(void *menu, const char *cmd, const char *subcmd, Jim_Interp *interp,int argc,Jim_Obj *const *argv) {
  if (argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "itemdetails ITEM_INDEX");
    return JIM_ERR;
  }
  long idx;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &idx) != JIM_OK, "%s %s: invalid index \"%s\"", cmd, subcmd, Jim_String(argv[0]));
  ITEM **items = menu_items(menu);
  int item_cnt = item_count(menu);
  RETURN_IF_ERR(idx >= item_cnt, "%s %s: item_index %d out of range", cmd, subcmd, idx);
  ITEM *it = items[idx];
  RETURN_IF_ERR(! it, "%s %s: item_index %d NULL", cmd, subcmd, idx);
  Jim_Obj *list = Jim_NewListObj(interp, NULL, 0);
  Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp, item_name(it), -1));
  Jim_ListAppendElement(interp, list, Jim_NewStringObj(interp, item_description(it), -1));
  Jim_SetResult(interp, list);
  return JIM_OK;    
}

static int wm_mark(void *menu, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "mark STR");
    return JIM_ERR;
  }
  const char *mark = Jim_String(argv[0]);
  RETURN_IF_ERR(set_menu_mark(menu, mark) != OK, "%s %s: unable to set mark: \"%s\"", cmd, subcmd, mark);
  return JIM_OK;    
}

static int wm_option(void *menu, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  /* Special order required: (o_selectable in uppercase only one in item)
   * Use these commands to get details:
   *     cat wmenu.c | ./ckhash code '(O_[A_Z_]+)' 0.7 1.5 > test1
   *     cat wmenu.c | ./ckhash stats '(O_[A_Z_]+)' 0.7 1.5 > test1 */
  static IntMacro MenuOptions[] = {
    {"O_ONEVALUE", O_ONEVALUE}, {"O_ROWMAJOR", O_ROWMAJOR},
    {NULL, 0}, {NULL, 0},
    {"O_SHOWDESC", O_SHOWDESC}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"O_NONCYCLIC", O_NONCYCLIC}, {"O_SHOWMATCH", O_SHOWMATCH},
    {"O_IGNORECASE", O_IGNORECASE}, {NULL, 0}, 
    {"O_MOUSE_MENU", O_MOUSE_MENU}, {NULL, 0}, 
  };
#undef NCOLS
#define NCOLS 2
  static int MenuOptionsNCols = NCOLS;
  static int MenuOptionsNRows = sizeof(MenuOptions) / sizeof(IntMacro)
	 / NCOLS;
  if (argc < 2) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "option on|off OPT1 ?...?");
    return JIM_ERR;
  }
  int opts = 0;
  for (int i = 1; i < argc; ++i) {
    const char *name = Jim_String(argv[i]);
    int single = getIntMacroVal(name, MenuOptions, MenuOptionsNRows, MenuOptionsNCols);
    RETURN_IF_ERR(single == -1, "%s %s: unknown menu option name: \"%s\"", cmd, subcmd, name);
    opts |= single; 
  }  
  const char *opstr = Jim_String(argv[0]);
  if (strcmp(opstr, "on") == 0) {
    RETURN_IF_ERR(menu_opts_on(menu, opts) != OK, "%s %s: failed to switch on menu options %s ...", cmd, subcmd, Jim_String(argv[1])); 
  } else if (strcmp(opstr, "off") == 0) {
    RETURN_IF_ERR(menu_opts_off(menu, opts) != OK, "%s %s: failed to switch off menu options %s ...", cmd, subcmd, Jim_String(argv[1])); 
  } else {
    RETURN_IF_ERR(1, "%s %s: \"%s\" unknown, should be \"on\" or \"off\"", cmd, subcmd, opstr);
  }
  return JIM_OK;
}

static int wm_post(void *menu, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "post");
    return JIM_ERR;
  }
  RETURN_IF_ERR(post_menu(menu) != OK, "%s %s: unable to post menu", cmd, subcmd); 
  return JIM_OK;    
}

static int wm_unpost(void *menu, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "unpost");
    return JIM_ERR;
  }
  RETURN_IF_NCERR(unpost_menu(menu)); 
  return JIM_OK;    
}
