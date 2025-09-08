// TODO check whether background can theoretically set color as well as highlight attr
static int wf_background(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wf_driver(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wf_fieldstr(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wf_foreground(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wf_formoption(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wf_justify(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wf_newpanel(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wf_fieldoption(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wf_padding(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wf_post(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wf_unpost(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);
static int wf_validate(void*,const char*,const char*,Jim_Interp*,int,Jim_Obj*const*);

static void FormDeleteCmd(Jim_Interp *interp, void *privData) {
  (void)interp;
  /* free memory in reverse to allocation process */
  if (privData) {
    FIELD **list = form_fields(privData);
    free_form(privData);
    for (int i = 0; list[i] != NULL; ++i) {
      free_field(list[i]);      
    }
    Jim_Free(list);
  }
}

static int FormCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  /* Special order required, get details using commands: 
   *     cat wform.c | ./ckhash code '\{"([a-z]+)",' 0.7 2.5 > test1
   *     cat wform.c | ./ckhash stats '\{"([a-z]+)",' 0.7 2.5 
   * Then wf_ (z_ for delete) needs to be manually added to the function names
   * optset (*set) is a field function, whereas setopt (set*) is a form 
   * function */
  static SubCmd FormSubcmds[] = {
    {"justify", wf_justify}, {NULL, 0}, 
    {"fieldstr", wf_fieldstr}, {"unpost", wf_unpost},
    {"driver", wf_driver}, {"newpanel", wf_newpanel},
    {"formoption", wf_formoption}, {NULL, 0}, 
    {"background", wf_background}, {"post", wf_post},
    {NULL, 0}, {NULL, 0},
    {"validate", wf_validate}, {NULL, 0}, 
    {"delete", z_delete}, {"foreground", wf_foreground},
    {NULL, 0}, {NULL, 0},
    {"fieldoption", wf_fieldoption}, {"padding", wf_padding},
  };
#undef NCOLS
#define NCOLS 2
  static int FormSubcmdsNCols = NCOLS;
  static int FormSubcmdsNRows = sizeof(FormSubcmds) / sizeof(SubCmd) / NCOLS;
  if (argc < 2) {
    Jim_WrongNumArgs(interp, 1, argv, "subcommand ?...?");
    return JIM_ERR;
  }
  const char *cmd = Jim_String(argv[0]);
  const char *subcmd = Jim_String(argv[1]);
  SubCmdFunc func = getSubCmd(subcmd, FormSubcmds, FormSubcmdsNRows, FormSubcmdsNCols);
  RETURN_IF_ERR(! func, "%s: unknown subcommand \"%s\"", cmd, subcmd);
  RETURN_IF_ERR(! initialized, "%s %s: no current (new) tui (?deleted)", cmd, subcmd);
  return func(Jim_CmdPrivData(interp), cmd, subcmd, interp, argc-2, argv+2);
}

static int wf_newform(void *form, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  (void)form;
  if (argc < 5 || argc % 5 != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "newform FIELD_HT WD YBEGIN XBEGIN HIDDENROWS ?... ... ... ... ...?");
    return JIM_ERR;
  }
  int nflist = argc / 5 + 1;
  FIELD **fields = Jim_Alloc(nflist * sizeof(FIELD*));
  long fval[5];
  int i;
  for (i = 0; i < nflist - 1; ++i) {
    for (int j = 0; j < 5; ++j) {
      RETURN_IF_ERR(Jim_GetLong(interp, argv[i * 5 + j], fval + j) != JIM_OK, "%s: arg %d not number", cmd, subcmd, i * 5 + j);
    }
    fields[i] = new_field(fval[0], fval[1], fval[2], fval[3], fval[4], 0);
    RETURN_IF_ERR(fields[i] == NULL, "%s %s: unable to create field %d", cmd, subcmd, i + 1);
  }
  fields[i] = NULL;
  FORM *new = new_form(fields);
  if (! new) {
    RETURN_IF_NCERR(errno);
  }
  static int id = 0;
  char name[MAX_CMD_NAME_LEN];
  snprintf(name, sizeof(name), "form%d", ++id);
  Jim_CreateCommand(interp, name, FormCmd, new, FormDeleteCmd);
  Jim_SetResultString(interp, name, -1);
  return JIM_OK;
}

// Subcommands

static int wf_driver(void *form, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  /* Special order required, get details using commands: 
   *     cat wform.c | ./ckhash code '\{"(REQ_[A-Z_]+)",' 0.7 2.5 > test1
   *     cat wform.c | ./ckhash stats '\{"(REQ_[A-Z_]+)",' 0.7 2.5 */
  static IntMacro FormDriverMacro[] = {
    {NULL, 0}, {NULL, 0},
    {"REQ_PREV_LINE", REQ_PREV_LINE}, {NULL, 0}, 
    {"REQ_BEG_LINE", REQ_BEG_LINE}, {NULL, 0}, 
    {"REQ_LAST_PAGE", REQ_LAST_PAGE}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"REQ_LEFT_FIELD", REQ_LEFT_FIELD}, {NULL, 0}, 
    {"REQ_SNEXT_FIELD", REQ_SNEXT_FIELD}, {NULL, 0}, 
    {"REQ_CLR_EOF", REQ_CLR_EOF}, {NULL, 0}, 
    {"REQ_LAST_FIELD", REQ_LAST_FIELD}, {"REQ_SCR_HBLINE", REQ_SCR_HBLINE},
    {NULL, 0}, {NULL, 0},
    {"REQ_SCR_BLINE", REQ_SCR_BLINE}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"REQ_CLR_EOL", REQ_CLR_EOL}, {NULL, 0}, 
    {"REQ_SPREV_FIELD", REQ_SPREV_FIELD}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"REQ_NEW_LINE", REQ_NEW_LINE}, {"REQ_SCR_FCHAR", REQ_SCR_FCHAR},
    {"REQ_DEL_WORD", REQ_DEL_WORD}, {"REQ_NEXT_WORD", REQ_NEXT_WORD},
    {NULL, 0}, {NULL, 0},
    {"REQ_END_FIELD", REQ_END_FIELD}, {NULL, 0}, 
    {"REQ_NEXT_CHOICE", REQ_NEXT_CHOICE}, {"REQ_VALIDATION", REQ_VALIDATION},
    {"REQ_DEL_LINE", REQ_DEL_LINE}, {"REQ_NEXT_LINE", REQ_NEXT_LINE},
    {NULL, 0}, {NULL, 0},
    {"REQ_SCR_FHPAGE", REQ_SCR_FHPAGE}, {"REQ_SCR_FPAGE", REQ_SCR_FPAGE},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"REQ_BEG_FIELD", REQ_BEG_FIELD}, {"REQ_INS_CHAR", REQ_INS_CHAR},
    {"REQ_SCR_HBHALF", REQ_SCR_HBHALF}, {"REQ_SFIRST_FIELD", REQ_SFIRST_FIELD},
    {"REQ_DOWN_FIELD", REQ_DOWN_FIELD}, {NULL, 0}, 
    {"REQ_UP_CHAR", REQ_UP_CHAR}, {NULL, 0}, 
    {"REQ_PREV_CHOICE", REQ_PREV_CHOICE}, {NULL, 0}, 
    {"REQ_FIRST_PAGE", REQ_FIRST_PAGE}, {"REQ_NEXT_FIELD", REQ_NEXT_FIELD},
    {"REQ_DEL_PREV", REQ_DEL_PREV}, {"REQ_RIGHT_FIELD", REQ_RIGHT_FIELD},
    {NULL, 0}, {NULL, 0},
    {"REQ_FIRST_FIELD", REQ_FIRST_FIELD}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"REQ_PREV_FIELD", REQ_PREV_FIELD}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"REQ_SCR_HFLINE", REQ_SCR_HFLINE}, {NULL, 0}, 
    {"REQ_INS_MODE", REQ_INS_MODE}, {NULL, 0}, 
    {"REQ_SCR_FLINE", REQ_SCR_FLINE}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"REQ_PREV_CHAR", REQ_PREV_CHAR}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"REQ_DOWN_CHAR", REQ_DOWN_CHAR}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"REQ_END_LINE", REQ_END_LINE}, {NULL, 0}, 
    {"REQ_INS_LINE", REQ_INS_LINE}, {"REQ_PREV_PAGE", REQ_PREV_PAGE},
    {NULL, 0}, {NULL, 0},
    {"REQ_SCR_BCHAR", REQ_SCR_BCHAR}, {NULL, 0}, 
    {"REQ_SLAST_FIELD", REQ_SLAST_FIELD}, {NULL, 0}, 
    {"REQ_OVL_MODE", REQ_OVL_MODE}, {NULL, 0}, 
    {"REQ_LEFT_CHAR", REQ_LEFT_CHAR}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"REQ_SCR_BPAGE", REQ_SCR_BPAGE}, {"REQ_SCR_HFHALF", REQ_SCR_HFHALF},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"REQ_SCR_BHPAGE", REQ_SCR_BHPAGE}, {NULL, 0}, 
    {"REQ_RIGHT_CHAR", REQ_RIGHT_CHAR}, {NULL, 0}, 
    {"REQ_DEL_CHAR", REQ_DEL_CHAR}, {"REQ_NEXT_CHAR", REQ_NEXT_CHAR},
    {NULL, 0}, {NULL, 0},
    {"REQ_CLR_FIELD", REQ_CLR_FIELD}, {"REQ_UP_FIELD", REQ_UP_FIELD},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0},
    {"REQ_PREV_WORD", REQ_PREV_WORD}, {NULL, 0}, 
    {"REQ_NEXT_PAGE", REQ_NEXT_PAGE}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
  };
#undef NCOLS
#define NCOLS 2
  static int FormDriverMacroNCols = NCOLS;
  static int FormDriverMacroNRows = sizeof(FormDriverMacro) / sizeof(IntMacro) / NCOLS;
  if (argc != 1) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "driver KEYNAME|CHAR");
    return JIM_ERR;
  }
  const char *key = Jim_String(argv[0]);
  if (strncmp(key, "REQ_", sizeof("REQ_")) == 0) {
    int macro = getIntMacroVal(key, FormDriverMacro, FormDriverMacroNRows, FormDriverMacroNCols);
    RETURN_IF_ERR(macro == -1, "%s %s: unknown driver code \"%s\"", cmd, subcmd, key);
    RETURN_IF_NCERR(form_driver_w(form, KEY_CODE_YES, macro));
  } else {
    long ch;
    RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &ch) != JIM_OK, "%s %s: \"%s\" not a codepoint", cmd, subcmd, key);
    RETURN_IF_NCERR(form_driver_w(form, 0, (wchar_t)ch)); // TODO instead of 0, may need to pass OK
  }
  return JIM_OK;
}

static int wf_background(void *form, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc < 2) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "background FIELD-IDX STYLE");
    return JIM_ERR;
  }
  long idx;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &idx) != JIM_OK, "%s %s: invalid field index %s", cmd, subcmd, Jim_String(argv[0])); 
  int nfields = field_count(form);
  RETURN_IF_ERR(idx >= nfields, "%s %s: field index out of range %d", cmd, subcmd, (int)idx);
  long style;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[1], &style) != JIM_OK, "%s %s: invalid field style %s", cmd, subcmd, Jim_String(argv[1])); 
  FIELD **fields = form_fields(form);
  RETURN_IF_NCERR(set_field_back(fields[idx], (chtype)style));
  return JIM_OK;
}

static int wf_fieldoption(void *form, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  /* Special order required, get details using commands: 
   *     cat wform.c | ./ckhash code '\{"(O_[A-Z][A-KM-RT-Z][A-Z_]+)",' 0.7 1.5 > test1
   *     cat wform.c | ./ckhash stats '\{"(O_[A-Z][A-KM-RT-Z][A-Z_]+)",' 0.7 1.5 */
  static UIntMacro FieldOptionMacro[] = {
    {"O_AUTOSKIP", O_AUTOSKIP}, {"O_STATIC", O_STATIC},
    {"O_DYNAMIC_JUSTIFY", O_DYNAMIC_JUSTIFY}, {NULL, 0}, 
    {NULL, 0}, {NULL, 0},
    {"O_EDIT", O_EDIT}, {"O_PASSOK", O_PASSOK},
    {"O_WRAP", O_WRAP}, {NULL, 0}, 
    {"O_BLANK", O_BLANK}, {"O_NO_LEFT_STRIP", O_NO_LEFT_STRIP},
    {"O_PUBLIC", O_PUBLIC}, {NULL, 0}, 
    {"O_ACTIVE", O_ACTIVE}, {NULL, 0}, 
    {"O_NULLOK", O_NULLOK}, {NULL, 0}, 
    {"O_INPUT_LIMIT", O_INPUT_LIMIT}, {"O_VISIBLE", O_VISIBLE},
    {"O_EDGE_INSERT_STAY", O_EDGE_INSERT_STAY}, {NULL, 0}, 
  };
#undef NCOLS
#define NCOLS 2
  static int FieldOptionMacroNCols = NCOLS;
  static int FieldOptionMacroNRows = sizeof(FieldOptionMacro) / sizeof(UIntMacro) / NCOLS;

  if (argc < 3) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "fieldoption on|off FIELD-IDX OPTIONMACRO ?...?");
    return JIM_ERR;
  }
  long idx;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[1], &idx) != JIM_OK, "%s %s: invalid field index %s", cmd, subcmd, Jim_String(argv[1])); 
  int nfields = field_count(form);
  RETURN_IF_ERR(idx >= nfields || idx < 0, "%s %s: field index out of range %d", cmd, subcmd, (int)idx);
  unsigned int opts = 0;
  for (int i = 2; i < argc; ++i) {
    const char *name = Jim_String(argv[i]);
    unsigned int single = getUIntMacroVal(name, FieldOptionMacro, FieldOptionMacroNRows, FieldOptionMacroNCols);
    RETURN_IF_ERR(single == (unsigned int)-1, "%s %s: unknown menu option name: \"%s\"", cmd, subcmd, name);
    opts |= single; 
  }  
  FIELD **fields = form_fields(form);
  const char *opstr = Jim_String(argv[0]);
  if (strcmp(opstr, "on") == 0) {
    RETURN_IF_NCERR(field_opts_on(fields[idx], opts)); 
  } else if (strcmp(opstr, "off") == 0) {
    RETURN_IF_NCERR(field_opts_off(fields[idx], opts)); 
  } else {
    RETURN_IF_ERR(1, "%s %s: \"%s\" unknown, should be \"on\" or \"off\"", cmd, subcmd, opstr);
  }
  return JIM_OK;
}

static int wf_fieldstr(void *form, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 1 && argc != 2) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "fieldstr FIELD-IDX ?STRING?");
    return JIM_ERR;
  }
  long idx;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &idx) != JIM_OK, "%s %s: invalid field index %s", cmd, subcmd, Jim_String(argv[0])); 
  int nfields = field_count(form);
  RETURN_IF_ERR(idx > nfields - 1, "%s %s: field index out of range %d", cmd, subcmd, (int)idx);
  FIELD **fields = form_fields(form);
  if (argc == 2) {
    const char *str = Jim_String(argv[1]);
    RETURN_IF_NCERR(set_field_buffer(fields[idx], 0, str)); 
  }
  Jim_SetResultString(interp, field_buffer(fields[idx], 0), -1);
  return JIM_OK;
}

static int wf_foreground(void *form, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc >= 2) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "foreground FIELD-IDX ATTR ?...?");
    return JIM_ERR;
  }
  long idx;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &idx) != JIM_OK, "%s %s: invalid field index %s", cmd, subcmd, Jim_String(argv[0])); 
  int nfields = field_count(form);
  RETURN_IF_ERR(idx >= nfields, "%s %s: field index out of range %d", cmd, subcmd, (int)idx);
  chtype attr = 0;
  for (int i = 1; i < argc; ++i) {
    const char *name = Jim_String(argv[i]);
    attr_t single = zgetattr(name);
    RETURN_IF_ERR(single == (attr_t)-1, "%s %s: unknown attribute name \"%s\"", cmd, subcmd, name);
    attr |= single; 
  }
  FIELD **fields = form_fields(form);
  RETURN_IF_NCERR(set_field_fore(fields[idx], attr));
  return JIM_OK;
}

static int wf_formoption(void *form, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
/* form options unsigned */
  /* Special order required, get details using commands: 
   *     cat wform.c | ./ckhash code '\{"(O_NL_[A-Z]+|O_BS_[A-Z]+)",' 0.7 1.5 > test1
   *     cat wform.c | ./ckhash stats '\{"(O_NL_[A-Z]+|O_BS_[A-Z]+)",' 0.7 1.5 */
  static UIntMacro FormOptionMacros[] = {
    {"O_BS_OVERLOAD", O_BS_OVERLOAD},
    {"O_NL_OVERLOAD", O_NL_OVERLOAD},
  };
#undef NCOLS
#define NCOLS 1
  static int FormOptionMacrosNCols = NCOLS;
  static int FormOptionMacrosNRows = sizeof(FormOptionMacros) / sizeof(UIntMacro) / NCOLS;
  if (argc < 2 || argc > 3) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "formoption on|off MACRO_NAME_1 ?MACRO_NAME_2?");
    return JIM_ERR;
  }
  const char *action = Jim_String(argv[0]);
  unsigned int macro[2] = {0};
  for (int i = 1; i < argc; ++i) {
    const char *name = Jim_String(argv[i]);
    macro[i-1] = getUIntMacroVal(name, FormOptionMacros, FormOptionMacrosNRows, FormOptionMacrosNCols);
    RETURN_IF_ERR(macro[i-1] == (unsigned int)-1, "%s %s: macro name \"%s\" not found", cmd, subcmd, name);
  }
  if (strcmp(action, "on") == 0) {
    for (int i = 0; i < 2; ++i) {
      if (macro[i] != 0) {
        RETURN_IF_NCERR(form_opts_on(form, macro[i]));
      }
    }
  } else if (strcmp(action, "off") == 0) {
    for (int i = 0; i < 2; ++i) {
      if (macro[i] != 0) {
        RETURN_IF_NCERR(form_opts_on(form, macro[i]));
      }
    }
  } else {
    Jim_SetResultFormatted(interp, "%s %s: unknown action \"%s\", should be on|off", cmd, subcmd, action);
    return JIM_ERR;
  }
  return JIM_OK;
}

static int wf_justify(void *form, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  /* Special order required, get details using commands: 
   *     cat wform.c | ./ckhash code '\{"(JUSTIFY_[A-Z]+|NO_[A-Z]+)",' 0.7 2.5 > test1
   *     cat wform.c | ./ckhash stats '\{"(JUSTIFY_[A-Z]+|NO_[A-Z]+)",' 0.7 2.5 */
  static IntMacro FieldJustMacro[] = {
    {"JUSTIFY_LEFT", JUSTIFY_LEFT},
    {"NO_JUSTIFICATION", NO_JUSTIFICATION},
    {"JUSTIFY_RIGHT", JUSTIFY_RIGHT},
    {NULL, 0},
    {"JUSTIFY_CENTER", JUSTIFY_CENTER},
  };
#undef NCOLS
#define NCOLS 1
  static int FieldJustMacroNCols = NCOLS;
  static int FieldJustMacroNRows = sizeof(FieldJustMacro) / sizeof(IntMacro) / NCOLS;
  if (argc != 2) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "justify FIELD-IDX MACRONAME");
    return JIM_ERR;
  }
  long idx;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &idx) != JIM_OK, "%s %s: invalid field index %s", cmd, subcmd, Jim_String(argv[0])); 
  int nfields = field_count(form);
  RETURN_IF_ERR(idx >= nfields, "%s %s: field index out of range %d", cmd, subcmd, (int)idx);
  const char *name = Jim_String(argv[1]);
  int macro = getIntMacroVal(name, FieldJustMacro, FieldJustMacroNRows, FieldJustMacroNCols);
  RETURN_IF_ERR(macro == -1, "%s %s: macro name \"%s\" unknown", cmd, subcmd, name);
  FIELD **fields = form_fields(form);
  RETURN_IF_NCERR(set_field_just(fields[idx], macro));
  return JIM_OK;
}

static int wf_newpanel(void *form, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 2 && argc != 6) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "newpanel YBEGIN XBEGIN ?TOP_MARGIN BOTTOM LEFT RIGHT?");
    return JIM_ERR;
  }
  long begy, begx;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &begy) != JIM_OK, "%s %s: invalid begin_y integer %s", cmd, subcmd, Jim_String(argv[0]));
  RETURN_IF_ERR(Jim_GetLong(interp, argv[1], &begx) != JIM_OK, "%s %s: invalid begin_x integer %s", cmd, subcmd, Jim_String(argv[1]));
  int oh, ow, ih, iw;
  RETURN_IF_ERR(scale_form(form, &ih, &iw) != OK, "%s %s: scale_form fail", cmd, subcmd);
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
  RETURN_IF_ERR(oh + begy >= maxy || ow + begx >= maxx, "%s %s: form extent (%d, %d) exceeds scr size (%d, %d)", cmd, subcmd, oh + begy, ow + begx, maxy, maxx);

  PanelData *new = Jim_Alloc(sizeof(PanelData));
  RETURN_IF_ERR(!new, "%s %s: failed to alloc PanelData", cmd, subcmd);
  new->sub = NULL;
  new->border_active = 0;
  WINDOW *win = newwin(oh, ow, begy, begx);
  RETURN_IF_ERR(!win, "%s %s: failed to create win", cmd, subcmd);
  keypad(win, TRUE);
  nodelay(win, noblock);// static bool set in tui.create
  PANEL *pnl = new_panel(win);
  RETURN_IF_ERR(! pnl, "%s %s: new_panel fail", cmd, subcmd);
  new->pnl = pnl;
  RETURN_IF_ERR(set_form_win(form, win) != OK, "%s %s: set_form_win fail", cmd, subcmd);
  const char *procname = "panel%d";
  if (argc == 6) {
    WINDOW *sub = derwin(win, ih, iw, top, left);
    RETURN_IF_ERR(! sub, "%s %s: derwin fail", cmd, subcmd);
    new->sub = sub;
    keypad(sub, TRUE);
    nodelay(win, noblock);// static bool set in tui.create
    RETURN_IF_ERR(set_form_sub(form, sub) != OK, "%s %s: set_form_sub fail", cmd, subcmd); 
    procname = "bpanel%d";
  }
  char name[MAX_CMD_NAME_LEN];
  static int id = 0;
  snprintf(name, MAX_CMD_NAME_LEN, procname, ++id);
  Jim_CreateCommand(interp, name, PanelCmd, new, PanelDeleteCmd);
  Jim_SetResultString(interp, name, -1);
  return JIM_OK;
}

static int wf_padding(void *form, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 2) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "padding FIELD-IDX CHAR");
    return JIM_ERR;
  }
  long idx;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &idx) != JIM_OK, "%s %s: invalid field index %s", cmd, subcmd, Jim_String(argv[0])); 
  int nfields = field_count(form);
  RETURN_IF_ERR(idx >= nfields || idx < 0, "%s %s: field index out of range %d", cmd, subcmd, (int)idx);
  FIELD **fields = form_fields(form);
  const char *ch = Jim_String(argv[1]);
  RETURN_IF_NCERR(set_field_pad(fields[idx], (int)*ch));
  return JIM_OK;
}

static int wf_post(void *form, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "post");
    return JIM_ERR;
  }
  RETURN_IF_NCERR(post_form(form)); 
  return JIM_OK;    
}

static int wf_validate(void *form, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc < 2) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "validate FIELD-IDX MACRO_NAME ?...?");
    return JIM_ERR;
  }
  long idx;
  RETURN_IF_ERR(Jim_GetLong(interp, argv[0], &idx) != JIM_OK, "%s %s: invalid field index %s", cmd, subcmd, Jim_String(argv[0])); 
  int nfields = field_count(form);
  RETURN_IF_ERR(idx >= nfields || idx < 0, "%s %s: field index out of range %d", cmd, subcmd, (int)idx);
  FIELD **fields = form_fields(form);
  const char *name = Jim_String(argv[1]);
  FIELDTYPE *macro = getFTMacroVal(name, FTMacro, FTMacroNRows, FTMacroNCols);
  RETURN_IF_ERR(macro == NULL, "%s %s: invalid macro name %s", cmd, subcmd, name);
  if (macro == TYPE_IPV4) {
    if (argc != 2) {
      Jim_WrongNumArgs(interp, 1, argv - 2, "validate IDX TYPE_IPV4");
      return JIM_ERR;
    }
    RETURN_IF_ERR(set_field_type(fields[idx], macro) != E_OK, "%s %s: unable to set validation %s", cmd, subcmd, name);
  } else if (macro == TYPE_ALPHA) {
    if (argc != 3) {
      Jim_WrongNumArgs(interp, 1, argv - 2, "validate IDX TYPE_ALPHA MINWIDTH");
      return JIM_ERR;
    }
    long minfieldwidth;
    RETURN_IF_ERR(Jim_GetLong(interp, argv[2], &minfieldwidth) != JIM_OK, "%s %s: invalid field index %s", cmd, subcmd, Jim_String(argv[2])); 
    RETURN_IF_ERR(set_field_type(fields[idx], macro, minfieldwidth) != E_OK, "%s %s: unable to set field (%d) validation %s", cmd, subcmd, (int)idx, name);
  } else if (macro == TYPE_ALNUM) {
    if (argc != 3) {
      Jim_WrongNumArgs(interp, 1, argv - 2, "validate IDX TYPE_ALNUM MINWIDTH");
      return JIM_ERR;
    }
    long minfieldwidth;
    RETURN_IF_ERR(Jim_GetLong(interp, argv[2], &minfieldwidth) != JIM_OK, "%s %s: invalid field index %s", cmd, subcmd, Jim_String(argv[2])); 
    RETURN_IF_ERR(set_field_type(fields[idx], macro, minfieldwidth) != E_OK, "%s %s: unable to set field (%d) validation %s", cmd, subcmd, (int)idx, name);
  } else if (macro == TYPE_REGEXP) {
    if (argc != 3) {
      Jim_WrongNumArgs(interp, 1, argv - 2, "validate IDX TYPE_REGEXP REGEX");
      return JIM_ERR;
    }
    const char *regex = Jim_String(argv[2]);
    RETURN_IF_ERR(set_field_type(fields[idx], macro, regex) != E_OK, "%s %s: unable to set field (%d) validation %s", cmd, subcmd, (int)idx, name);
  } else if (macro == TYPE_NUMERIC) {
    if (argc != 5) {
      Jim_WrongNumArgs(interp, 1, argv - 2, "validate IDX TYPE_NUMERIC PRECISION MIN MAX");
      return JIM_ERR;
    }
    long prec, min, max;
    RETURN_IF_ERR(Jim_GetLong(interp, argv[2], &prec) != JIM_OK, "%s %s: invalid precision %s", cmd, subcmd, Jim_String(argv[2])); 
    RETURN_IF_ERR(Jim_GetLong(interp, argv[3], &min) != JIM_OK, "%s %s: invalid minimum %s", cmd, subcmd, Jim_String(argv[3])); 
    RETURN_IF_ERR(Jim_GetLong(interp, argv[4], &max) != JIM_OK, "%s %s: invalid maximum %s", cmd, subcmd, Jim_String(argv[4])); 
    RETURN_IF_ERR(set_field_type(fields[idx], macro, (int)prec, min, max) != E_OK, "%s %s: unable to set field (%d) validation %s", cmd, subcmd, (int)idx, name);
  } else if (macro == TYPE_INTEGER) {
    if (argc != 5) {
      Jim_WrongNumArgs(interp, 1, argv - 2, "validate IDX TYPE_INTEGER PRECISION MIN MAX");
      return JIM_ERR;
    }
    long prec, min, max;
    RETURN_IF_ERR(Jim_GetLong(interp, argv[2], &prec) != JIM_OK, "%s %s: invalid precision %s", cmd, subcmd, Jim_String(argv[2])); 
    RETURN_IF_ERR(Jim_GetLong(interp, argv[3], &min) != JIM_OK, "%s %s: invalid minimum %s", cmd, subcmd, Jim_String(argv[3])); 
    RETURN_IF_ERR(Jim_GetLong(interp, argv[4], &max) != JIM_OK, "%s %s: invalid maximum %s", cmd, subcmd, Jim_String(argv[4])); 
    RETURN_IF_ERR(set_field_type(fields[idx], macro, (int)prec, min, max) != E_OK, "%s %s: unable to set field (%d) validation %s", cmd, subcmd, (int)idx, name);
  } else if (macro == TYPE_ENUM) {
    if (argc < 5) {
      Jim_WrongNumArgs(interp, 1, argv - 2, "validate IDX TYPE_ENUM CASE-SENSITIVE UNIQUE-PARTIAL-MATCH ENUM_NAME ?...?");
      return JIM_ERR;
    }
    long casesens, partial;
    RETURN_IF_ERR(Jim_GetLong(interp, argv[2], &casesens) != JIM_OK || (casesens != 0 && casesens != 1), "%s %s: invalid case-sensitivity bool %s", cmd, subcmd, Jim_String(argv[2])); 
    RETURN_IF_ERR(Jim_GetLong(interp, argv[3], &partial) != JIM_OK || (partial != 0 && partial != 1), "%s %s: invalid unique partial match bool %s", cmd, subcmd, Jim_String(argv[3])); 
    const char *enums[argc - 3]; // argc - 4 + 1 for list terminating NULL
    for (int i = 4; i < argc; ++i) {
      enums[i - 4] = Jim_String(argv[i]);
    }
    enums[argc - 4] = NULL;
    RETURN_IF_ERR(set_field_type(fields[idx], macro, enums, (int)casesens, (int)partial) != E_OK, "%s %s: unable to set field (%d) validation %s", cmd, subcmd, (int)idx, name);
  }
  return JIM_OK;
}

static int wf_unpost(void *form, const char *cmd, const char *subcmd, Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc != 0) {
    Jim_WrongNumArgs(interp, 1, argv - 2, "unpost");
    return JIM_ERR;
  }
  RETURN_IF_NCERR(unpost_form(form)); 
  return JIM_OK;    
}
