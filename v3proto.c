/*	v3proto - Prototypes for V3				*/

/*	V 3 D R I V E R		*/

int v3_driver_option_handler(char *,char *) ;


/*	V 3 E D T U			*/

int xti_int() ;
int itx_int() ;
int xti_alpha(int) ;
int xti_date() ;
int xti_date_num() ;
int itx_date() ;
int xti_date_time() ;
int itx_date_time() ;
void xti_gen() ;
void itx_gen() ;
int xti_logical() ;
int itx_logical() ;
int xti_month() ;
int itx_month() ;
int xti_pointer() ;
int itx_pointer() ;
int xti_time() ;
int itx_time() ;

/*	V 3 I O U		*/

void iou_open(struct iou__unit *) ;
void iou_close_all_trap() ;
void iou_close_all() ;
void iou_close(struct iou__unit *, char *) ;
void iou_get(struct iou__unit *) ;
void iou_put(struct iou__unit *) ;
void iou_misc() ;
void iou_ctrlp(int) ;
#ifdef ISOCKETS
void iou_OpenTerminalSocket(char *,int,int) ;
#endif
int iou_input_tty(char *,char *,int,int) ;
char *iou_scr_ansi() ;
void iou_tty_misc() ;
int iou_scr_get(char *,int,int,int,char *) ;
#ifdef WINNT
int iou_ReadEvent(HANDLE,int,int,int) ;
int termGetConsoleMode(HANDLE,int*) ;
int termWriteFile(HANDLE,char *,int,int *) ;
#endif
void iou_scr_display_rows() ;
int rptu_colpos(struct str__ref *,int *,int,int) ;
int rptu_colwidth() ;
void rptu_lineupd(struct rpt__line *,int) ;
void sys_queue(char *) ;
void iou_seg_create() ;
void iou_seg_lock() ;
void iou_seg_share() ;
void iou_seg_unlock() ;
void iou_seg_update() ;

/*	V 3 M S C U		*/

void mscu_date_info(struct v3__date_table *) ;
int mscu_flag_lookup(char *) ;
void vec_add() ;
void vec_add_constant() ;
void vec_copy() ;
void vec_find_constant() ;
void vec_set() ;
void vec_span() ;
void vec_sum() ;
void vec_swap() ;


/*	V 3 M T H U		*/

void mthu_cvtpp() ;
void mthu_cvtps() ;
void mthu_mulpd() ;
void mthu_cvtip() ;
void mthu_addpd() ;
void mthu_cvtis(B64INT,int,char *,int,int) ;
void mthu_negpd() ;
void mthu_cvtsp() ;
B64INT mthu_cvtii(B64INT,int,int) ;
void mthu_cvtss(char *,int,int,char *,int,int) ;
void mthu_cvtsv(char *,int,int,union val__v3num *) ;
void mthu_cvtid(B64INT,int,double *) ;
int mthu_cvtsi(char *,int,int,int) ;
int mthu_cvtdi(double *,int) ;
void mthu_cmppd() ;
void mthu_divpd() ;
void mthu_percpd() ;
void mthu_subpd() ;
int mthu_cvtpi() ;
void mthu_cvtatr(struct num__ref *,double *) ;
void mthu_cvtvs(union val__v3num *,char *,int,int) ;
char *v3num_cvtcb(union val__v3num *,int,int *) ;
int v3num_popv3n(union val__v3num *) ;
int v3num_cmp(union val__v3num *,union val__v3num *) ;
B64INT xctu_popint() ;
void v3num_mdas(union val__v3num *,union val__v3num *,int) ;
void v3num_itx(union val__v3num *,char *,int) ;


/*	V 3 O P E R		*/

int v3_operations(int) ;

/*	V 3 O P E R X		*/

void v3_operationsx(int) ;


/*	V 3 P C K U		*/

void pcku_parse_init(int,char *) ;
void pcku_parse_load(char *) ;
struct db__package *pcku_xct_load(char *,int) ;
void pcku_save(struct db__package *,char *) ;
void pcku_unload(struct db__package *) ;
void pcku_load_package(struct db__package *) ;
void pcku_load_module(struct db__module_entry *) ;
int pcku_module_unload(struct db__module_entry *) ;
struct db__module_entry *pcku_module_defined(char *) ;
struct db__package *pcku_find(char *,int) ;
void fork_delete() ;
void fork_new() ;
void fork_switch() ;
void dbg_break() ;
void dbg_clearbp() ;
int dbg_inst_length(V3OPEL) ;
void dbg_setbp() ;
void dbg_show_calls() ;

/*	V 3 P R S A		*/

int prsu_stash_lit(int) ;
int prsu_stash_pure(char *,int) ;
int prsu_alloc_impure(int,int) ;
int prsu_alloc_pure(int,int) ;
int prsu_alloc_ob(int,int) ;
int prsu_alloc_code(int,int) ;
int prsu_alloc_stack(int,int) ;
int prsu_alloc_struct(int,int) ;
int prsu_formref(int,int,int,int) ;
int prsu_valref(int,int) ;
char *prsu_predefined_lookup(char *,struct db__predef_table *,int) ;
struct db__prs_sym *prsu_sym_add(char *,unsigned char) ;
struct db__prs_sym *prsu_sym_lookup(char *,int) ;
struct db__prs_sym *prsu_sym_define(char *,int,int,int,int,int,int,int,int,int,int,int,int) ;
int prsu_str_hash(unsigned char *,int) ;
void prsu_array_setup(char [],char [],char) ;
void prsu_nest_input(struct db__parse_info *,FILE *,char *) ;
struct db__parse_info *prsu_init_new() ;
void prsu_checksum_table_insert(char,char *) ;
void prsu_gen_symbol_ref(struct prs__symbol_ref_list *) ;
void prsu_chk_oper(struct db__opr_info *,int,int,int) ;
void prsu_chk_value(struct db__prs_sym *) ;
void prsu_show_size() ;
void prsu_error(int,char *,char *) ;

/*	V 3 P R S B		*/

char *prsu_nxt_token(int) ;
void prsu_nxt_line() ;
int prsu_eval_constant(int,int) ;
void prs_begin_level(char *,int,int,int) ;
void prs_end_level(char *) ;
struct db__prs_level *prsu_find_level() ;
void prsu_parser() ;
void prsu_parser_symbol(struct db__prs_sym *,int) ;
void prsu_V4IsctMacro(char *,int) ;

/*	V 3 P R S C		*/

int prsu_dcl(int) ;
int prs_attempt_struct_load(char *) ;

/*	V 3 P R S D		*/

int prsu_dcl_subs(struct db__dsd *) ;
void prsu_dcl_constant(int,int) ;
int prsu_eval_int_exp(int,int *) ;
void prsu_eval_intexp_ops(int) ;
void prsu_dcl_obj() ;
void prsu_dcl_obj_value(struct db__dcl_object *) ;
void prsu_dcl_getob(struct db__dcl_objref *,char *) ;
int prsu_dcl_radix() ;
int prsu_dcl_checksums() ;
int prsu_dcl_flag() ;
int prsu_dcl_include() ;
int prsu_dcl_v4eval() ;
int prsu_dcl_v4modulebind() ;
int prsu_dcl_v4moduleeval() ;
int prsu_dcl_v4structeval() ;
int prsu_dcl_v4b() ;
int prsu_dcl_reference_checking() ;
int prsu_dcl_startup_module() ;
int prsu_dcl_package_id() ;
int prsu_dcl_package_slot() ;
int prsu_dcl_assertions() ;

/*	V 3 S O B U		*/

void sobu_calculate_name_ob_list() ;
void sobu_insert_name_ob(struct db__dcl_object *,int) ;
int sobu_name_create(char *,int) ;
int sobu_name_lookup(char *,int) ;
int sobu_name_tran(union val__sob,int) ;
//int sobu_has_find(union val__sob,union val__sob) ;
int sobu_has_find(int,int) ;
struct ob__desc *sobu_object_create(struct db__dcl_object *,int) ;
//void sobu_bucket_clean(struct db__ob_bucket *) ;
//int sobu_object_find(union val__sob) ;
int sobu_object_find(int) ;
//int sobu_object_dereference(union val__sob,char *) ;
int sobu_object_dereference(int,char *) ;
//struct ob__desc *sobu_pointer(union val__sob) ;
struct ob__desc *sobu_pointer(int) ;
//char *sobu_name_str(union val__sob) ;
char *sobu_name_str(int) ;
int sobu_assertion_add(char *) ;
int sobu_assertion_delete(char *) ;
int sobu_assertion_test(char *) ;
//struct db__dcl_valref *sobu_object_value(union val__sob) ;
struct db__dcl_valref *sobu_object_value(int) ;
//int sobu_push_val(union val__sob,int,char *,struct db__dcl_objref *,int,int) ;
int sobu_push_val(int,int,char *,struct db__dcl_objref *,int,int) ;
//int sobu_val_num(union val__sob) ;
int sobu_val_num(int) ;
void sobu_bind_skeleton() ;
//void sobu_load_skeleton(struct db__pck_skeletons *,struct db__prs_skeleton *,union val__sob) ;
void sobu_load_skeleton(struct db__pck_skeletons *,struct db__prs_skeleton *,int) ;

/*	V 3 S T R U		*/

void stru_append() ;
void stru_be() ;
void stru_be2() ;
void stru_bl() ;
void stru_break() ;
void stru_break_right() ;
void stru_compare() ;
void stru_compress() ;
void stru_concat() ;
void stru_expand() ;
void stru_itos() ;
void stru_len() ;
void stru_list() ;
void stru_logical() ;
void stru_match() ;
void stru_nulls() ;
void stru_prior() ;
void stru_share() ;
void stru_spaces() ;
void stru_trim() ;
void stru_type() ;
void stru_updfix() ;
void stru_updind() ;
void stru_updvar() ;
char *stru_alloc_bytes(int) ;
void stru_csuc(char *) ;
int stru_sslen(struct str__ref *) ;
char *stru_cvtcstr(char *,struct str__ref *) ;
char *stru_popcstr(char *) ;
void stru_popstr(struct str__ref *) ;

/*	V 3 V 4		*/

int v3_v4_handler(int) ;
int v3v4_UpdateArg(struct V4DPI__Point *,struct V4C__Context *) ;
struct V4DPI__Point *v3_v4_EvalV3Mod(struct V4DPI__Point *,struct V4C__Context *,struct V4DPI__Point *) ;
int xti_point(struct V4C__Context *) ;
int itx_point(struct V4C__Context *) ;
struct V4C__Context *v3v4_GetV4ctx() ;
void v3v4_v4im_Load_V3PicMod(int,int) ;
void v3v4_v4im_Unload_V3PicMod(char *) ;
void vrpp_Execute(struct V4C__Context *,struct VRPP__Xct *) ;


/*	V 3 X C T U		*/

struct db__process *process_init_new() ;
void xctu_main() ;
void xctu_call(int) ;
struct db__module_entry *xctu_call_module_entry(int,char *,struct db__module_runtime_entry **) ;
int xctu_attempt_v4_load(char *) ;
void xctu_call_defer(struct db__formatptr *) ;
void xctu_call_mod(struct db__module_entry *) ;
void xctu_call_parser(int,FILE *,char *,int) ;
int xctu_return() ;
struct db__mod_item *xctu_mod_item_alloc() ;
void xctu_mod_item_delete() ;
xctu_mod_item_exit(struct db__psi *,int) ;
void xctu_mod_item_find() ;
void xctu_mod_item_fork(int) ;
void xctu_mod_item_free(struct db__mod_item *) ;
void xctu_mod_item_push() ;
void xctu_mod_watchdog() ;
void xctu_cmd_str_convert(char *,struct db__command_search_list *) ;
int xctu_cmd_find(struct db__command_search_list *,struct db__xct_cmd_info *) ;
struct db__psi *xctu_cmd_invoke(struct db__xct_cmd_info *,struct cmd__args *) ;
void xctu_popnum(struct num__ref *) ;
char *xctu_popptr() ;
//char *xctu_pointer(union val__mempos) ;
char *xctu_pointer(int) ;
void xctu_num_upd(B64INT,int,struct num__ref *) ;
void sigint_exit_handler(int) ;
void exit_handler(void) ;
void unix_ctrlp() ;
void ctrlp(int) ;
void exit_handler() ;
void sigint_exit_handler(int) ;
void iou_close_all_trap(int) ;
void v3_signal_quit(int) ;
void v3_signal_ill(int) ;
void v3_signal_fpe(int) ;
void v3_signal_bus(int) ;
void intu_call_interrupt(int,struct cmd__args *) ;
void intu_got_alarm() ;
void intu_disable(int) ;
void intu_enable(int) ;
void intu_got_ctrlc(int) ;
void intu_ignore(int) ;
void intu_set_timer(int) ;
void v3_error(int,char *,char *,char *,char *,void *) ;



