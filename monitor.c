#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "simulator.h"
#include "io_com_udp.h"
#include "io_file.h"
#include "symtab.h"
#include "monitor.h"
#include "command.h"
#include "bus_access.h"
#include "os9syscalls.h"
#include "machine.h"
#include "logging.h"

#define S_NAMED 0x1
#define S_OFFSET 0x2
#define S_LINE 0x4

#define MAX_BREAKPOINTS 16

#define MAX_FUNCTION_CALLS 512

#define BP_FREE 0x0
#define BP_USED 0x1
#define BP_TEMP 0x2

#define PROMPT_REGS 0x1
#define PROMPT_CYCLES 0x2
#define PROMPT_INSN 0x4

#define COMMAND_SRC_CLIENT_PORT 9002
#define COMMAND_DST_SERVER_PORT 7403
#define COMMAND_SRC_SERVER_PORT 9004

struct symbol_table {
	struct symbol *addr_to_symbol[0x10000];
	char *name_area;
	int name_area_free;
	char *name_area_next;
};


struct breakpoint {
	target_addr_t addr;
	int flags;
	int count;
};

/* The function call stack */
struct function_call fctab[MAX_FUNCTION_CALLS];

/* The top of the function call stack */
struct function_call *current_function_call;

/* Automatically break after executing this many instructions */
int auto_break_insn_count = 0;

/* Debug status */
BOOLEAN debug_st = DEACTIVATED;

int dump_every_insn = 0;

enum addr_mode
{
  _illegal, _implied, _imm_byte, _imm_word, _direct, _extended,
  _indexed, _rel_byte, _rel_word, _reg_post, _sys_post, _usr_post
};

enum opcode
{
  _undoc, _abx, _adca, _adcb, _adda, _addb, _addd, _anda, _andb,
  _andcc, _asla, _aslb, _asl, _asra, _asrb, _asr, _bcc, _lbcc,
  _bcs, _lbcs, _beq, _lbeq, _bge, _lbge, _bgt, _lbgt, _bhi,
  _lbhi, _bita, _bitb, _ble, _lble, _bls, _lbls, _blt, _lblt,
  _bmi, _lbmi, _bne, _lbne, _bpl, _lbpl, _bra, _lbra, _brn,
  _lbrn, _bsr, _lbsr, _bvc, _lbvc, _bvs, _lbvs, _clra, _clrb,
  _clr, _cmpa, _cmpb, _cmpd, _cmps, _cmpu, _cmpx, _cmpy, _coma,
  _comb, _com, _cwai, _daa, _deca, _decb, _dec, _eora, _eorb,
  _exg, _inca, _incb, _inc, _jmp, _jsr, _lda, _ldb, _ldd,
  _lds, _ldu, _ldx, _ldy, _leas, _leau, _leax, _leay, _lsra,
  _lsrb, _lsr, _mul, _nega, _negb, _neg, _nop, _ora, _orb,
  _orcc, _pshs, _pshu, _puls, _pulu, _rola, _rolb, _rol, _rora,
  _rorb, _ror, _rti, _rts, _sbca, _sbcb, _sex, _sta, _stb,
  _std, _sts, _stu, _stx, _sty, _suba, _subb, _subd, _swi,
  _swi2, _swi3, _sync, _tfr, _tsta, _tstb, _tst, _reset,
#ifdef H6309
  _negd, _comd, _lsrd, _rord, _asrd, _rold, _decd, _incd, _tstd,
  _clrd
#endif
};

char *mne[] = {
  "???", "ABX", "ADCA", "ADCB", "ADDA", "ADDB", "ADDD", "ANDA", "ANDB",
  "ANDCC", "ASLA", "ASLB", "ASL", "ASRA", "ASRB", "ASR", "BCC", "LBCC",
  "BCS", "LBCS", "BEQ", "LBEQ", "BGE", "LBGE", "BGT", "LBGT", "BHI",
  "LBHI", "BITA", "BITB", "BLE", "LBLE", "BLS", "LBLS", "BLT", "LBLT",
  "BMI", "LBMI", "BNE", "LBNE", "BPL", "LBPL", "BRA", "LBRA", "BRN",
  "LBRN", "BSR", "LBSR", "BVC", "LBVC", "BVS", "LBVS", "CLRA", "CLRB",
  "CLR", "CMPA", "CMPB", "CMPD", "CMPS", "CMPU", "CMPX", "CMPY", "COMA",
  "COMB", "COM", "CWAI", "DAA", "DECA", "DECB", "DEC", "EORA", "EORB",
  "EXG", "INCA", "INCB", "INC", "JMP", "JSR", "LDA", "LDB", "LDD",
  "LDS", "LDU", "LDX", "LDY", "LEAS", "LEAU", "LEAX", "LEAY", "LSRA",
  "LSRB", "LSR", "MUL", "NEGA", "NEGB", "NEG", "NOP", "ORA", "ORB",
  "ORCC", "PSHS", "PSHU", "PULS", "PULU", "ROLA", "ROLB", "ROL", "RORA",
  "RORB", "ROR", "RTI", "RTS", "SBCA", "SBCB", "SEX", "STA", "STB",
  "STD", "STS", "STU", "STX", "STY", "SUBA", "SUBB", "SUBD", "SWI",
  "SWI2", "SWI3", "SYNC", "TFR", "TSTA", "TSTB", "TST", "RESET",
#ifdef H6309
  "NEGD", "COMD", "LSRD", "RORD", "ASRD", "ROLD", "DECD",
  "INCD", "TSTD", "CLRD",
#endif
};

typedef struct
{
  UINT8 code;
  UINT8 mode;
} opcode_t;

opcode_t codes[256] = {
  {_neg, _direct},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_com, _direct},
  {_lsr, _direct},
  {_undoc, _illegal},
  {_ror, _direct},
  {_asr, _direct},
  {_asl, _direct},
  {_rol, _direct},
  {_dec, _direct},
  {_undoc, _illegal},
  {_inc, _direct},
  {_tst, _direct},
  {_jmp, _direct},
  {_clr, _direct},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_nop, _implied},
  {_sync, _implied},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_lbra, _rel_word},
  {_lbsr, _rel_word},
  {_undoc, _illegal},
  {_daa, _implied},
  {_orcc, _imm_byte},
  {_undoc, _illegal},
  {_andcc, _imm_byte},
  {_sex, _implied},
  {_exg, _reg_post},
  {_tfr, _reg_post},
  {_bra, _rel_byte},
  {_brn, _rel_byte},
  {_bhi, _rel_byte},
  {_bls, _rel_byte},
  {_bcc, _rel_byte},
  {_bcs, _rel_byte},
  {_bne, _rel_byte},
  {_beq, _rel_byte},
  {_bvc, _rel_byte},
  {_bvs, _rel_byte},
  {_bpl, _rel_byte},
  {_bmi, _rel_byte},
  {_bge, _rel_byte},
  {_blt, _rel_byte},
  {_bgt, _rel_byte},
  {_ble, _rel_byte},
  {_leax, _indexed},
  {_leay, _indexed},
  {_leas, _indexed},
  {_leau, _indexed},
  {_pshs, _sys_post},
  {_puls, _sys_post},
  {_pshu, _usr_post},
  {_pulu, _usr_post},
  {_undoc, _illegal},
  {_rts, _implied},
  {_abx, _implied},
  {_rti, _implied},
  {_cwai, _imm_byte},
  {_mul, _implied},
  {_reset, _implied},
  {_swi, _implied},
  {_nega, _implied},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_coma, _implied},
  {_lsra, _implied},
  {_undoc, _illegal},
  {_rora, _implied},
  {_asra, _implied},
  {_asla, _implied},
  {_rola, _implied},
  {_deca, _implied},
  {_undoc, _illegal},
  {_inca, _implied},
  {_tsta, _implied},
  {_undoc, _illegal},
  {_clra, _implied},
  {_negb, _implied},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_comb, _implied},
  {_lsrb, _implied},
  {_undoc, _illegal},
  {_rorb, _implied},
  {_asrb, _implied},
  {_aslb, _implied},
  {_rolb, _implied},
  {_decb, _implied},
  {_undoc, _illegal},
  {_incb, _implied},
  {_tstb, _implied},
  {_undoc, _illegal},
  {_clrb, _implied},
  {_neg, _indexed},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_com, _indexed},
  {_lsr, _indexed},
  {_undoc, _illegal},
  {_ror, _indexed},
  {_asr, _indexed},
  {_asl, _indexed},
  {_rol, _indexed},
  {_dec, _indexed},
  {_undoc, _illegal},
  {_inc, _indexed},
  {_tst, _indexed},
  {_jmp, _indexed},
  {_clr, _indexed},
  {_neg, _extended},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_com, _extended},
  {_lsr, _extended},
  {_undoc, _illegal},
  {_ror, _extended},
  {_asr, _extended},
  {_asl, _extended},
  {_rol, _extended},
  {_dec, _extended},
  {_undoc, _illegal},
  {_inc, _extended},
  {_tst, _extended},
  {_jmp, _extended},
  {_clr, _extended},
  {_suba, _imm_byte},
  {_cmpa, _imm_byte},
  {_sbca, _imm_byte},
  {_subd, _imm_word},
  {_anda, _imm_byte},
  {_bita, _imm_byte},
  {_lda, _imm_byte},
  {_undoc, _illegal},
  {_eora, _imm_byte},
  {_adca, _imm_byte},
  {_ora, _imm_byte},
  {_adda, _imm_byte},
  {_cmpx, _imm_word},
  {_bsr, _rel_byte},
  {_ldx, _imm_word},
  {_undoc, _illegal},
  {_suba, _direct},
  {_cmpa, _direct},
  {_sbca, _direct},
  {_subd, _direct},
  {_anda, _direct},
  {_bita, _direct},
  {_lda, _direct},
  {_sta, _direct},
  {_eora, _direct},
  {_adca, _direct},
  {_ora, _direct},
  {_adda, _direct},
  {_cmpx, _direct},
  {_jsr, _direct},
  {_ldx, _direct},
  {_stx, _direct},
  {_suba, _indexed},
  {_cmpa, _indexed},
  {_sbca, _indexed},
  {_subd, _indexed},
  {_anda, _indexed},
  {_bita, _indexed},
  {_lda, _indexed},
  {_sta, _indexed},
  {_eora, _indexed},
  {_adca, _indexed},
  {_ora, _indexed},
  {_adda, _indexed},
  {_cmpx, _indexed},
  {_jsr, _indexed},
  {_ldx, _indexed},
  {_stx, _indexed},
  {_suba, _extended},
  {_cmpa, _extended},
  {_sbca, _extended},
  {_subd, _extended},
  {_anda, _extended},
  {_bita, _extended},
  {_lda, _extended},
  {_sta, _extended},
  {_eora, _extended},
  {_adca, _extended},
  {_ora, _extended},
  {_adda, _extended},
  {_cmpx, _extended},
  {_jsr, _extended},
  {_ldx, _extended},
  {_stx, _extended},
  {_subb, _imm_byte},
  {_cmpb, _imm_byte},
  {_sbcb, _imm_byte},
  {_addd, _imm_word},
  {_andb, _imm_byte},
  {_bitb, _imm_byte},
  {_ldb, _imm_byte},
  {_undoc, _illegal},
  {_eorb, _imm_byte},
  {_adcb, _imm_byte},
  {_orb, _imm_byte},
  {_addb, _imm_byte},
  {_ldd, _imm_word},
  {_undoc, _illegal},
  {_ldu, _imm_word},
  {_undoc, _illegal},
  {_subb, _direct},
  {_cmpb, _direct},
  {_sbcb, _direct},
  {_addd, _direct},
  {_andb, _direct},
  {_bitb, _direct},
  {_ldb, _direct},
  {_stb, _direct},
  {_eorb, _direct},
  {_adcb, _direct},
  {_orb, _direct},
  {_addb, _direct},
  {_ldd, _direct},
  {_std, _direct},
  {_ldu, _direct},
  {_stu, _direct},
  {_subb, _indexed},
  {_cmpb, _indexed},
  {_sbcb, _indexed},
  {_addd, _indexed},
  {_andb, _indexed},
  {_bitb, _indexed},
  {_ldb, _indexed},
  {_stb, _indexed},
  {_eorb, _indexed},
  {_adcb, _indexed},
  {_orb, _indexed},
  {_addb, _indexed},
  {_ldd, _indexed},
  {_std, _indexed},
  {_ldu, _indexed},
  {_stu, _indexed},
  {_subb, _extended},
  {_cmpb, _extended},
  {_sbcb, _extended},
  {_addd, _extended},
  {_andb, _extended},
  {_bitb, _extended},
  {_ldb, _extended},
  {_stb, _extended},
  {_eorb, _extended},
  {_adcb, _extended},
  {_orb, _extended},
  {_addb, _extended},
  {_ldd, _extended},
  {_std, _extended},
  {_ldu, _extended},
  {_stu, _extended}
};

opcode_t codes10[256] = {
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_lbrn, _rel_word},
  {_lbhi, _rel_word},
  {_lbls, _rel_word},
  {_lbcc, _rel_word},
  {_lbcs, _rel_word},
  {_lbne, _rel_word},
  {_lbeq, _rel_word},
  {_lbvc, _rel_word},
  {_lbvs, _rel_word},
  {_lbpl, _rel_word},
  {_lbmi, _rel_word},
  {_lbge, _rel_word},
  {_lblt, _rel_word},
  {_lbgt, _rel_word},
  {_lble, _rel_word},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_swi2, _implied},
  {_undoc, _illegal}, /* 10 40 */
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmpd, _imm_word},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmpy, _imm_word},
  {_undoc, _illegal},
  {_ldy, _imm_word},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmpd, _direct},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmpy, _direct},
  {_undoc, _illegal},
  {_ldy, _direct},
  {_sty, _direct},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmpd, _indexed},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmpy, _indexed},
  {_undoc, _illegal},
  {_ldy, _indexed},
  {_sty, _indexed},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmpd, _extended},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmpy, _extended},
  {_undoc, _illegal},
  {_ldy, _extended},
  {_sty, _extended},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_lds, _imm_word},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_lds, _direct},
  {_sts, _direct},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_lds, _indexed},
  {_sts, _indexed},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_lds, _extended},
  {_sts, _extended}
};

opcode_t codes11[256] = {
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_swi3, _implied},
  {_undoc, _illegal}, /* 11 40 */
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmpu, _imm_word},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmps, _imm_word},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmpu, _direct},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmps, _direct},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmpu, _indexed},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmps, _indexed},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmpu, _extended},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_cmps, _extended},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal}
};

char *reg[] = {
  "D", "X", "Y", "U", "S", "PC", "??", "??",
  "A", "B", "CC", "DP", "??", "??", "??", "??"
};

char index_reg[] = { 'X', 'Y', 'U', 'S' };

char *off4[] = {
  "0", "1", "2", "3", "4", "5", "6", "7",
  "8", "9", "10", "11", "12", "13", "14", "15",
  "-16", "-15", "-14", "-13", "-12", "-11", "-10", "-9",
  "-8", "-7", "-6", "-5", "-4", "-3", "-2", "-1"
};

int command_server, command_client;

const char* absolute_addr_name (absolute_address_t addr)
{
   static char buf[256], *bufptr;
   const char *name;

   bufptr = buf;

   bufptr += sprintf (bufptr, "%02lX:0x%04lX", addr >> 28, addr & 0xFFFFFF);

   name = sym_lookup (PROGRAM_SYMTAB_T, addr);
   if (name)
      sprintf (bufptr, "  <%-16.16s>", name);

   return buf;
}

void monitor_set_debug(BOOLEAN status)
{
	debug_st = status;
}

BOOLEAN monitor_get_debug_status (void)
{
	return debug_st;
}


/* Disassemble the current instruction.  Returns the number of bytes that
compose it. */
int dasm (char *buf, absolute_address_t opc)
{
  UINT8 op, am;
  char *op_str;
  absolute_address_t pc = opc;
  char R;
  int fetch1;			/* the first (MSB) fetched byte, used in macro RDWORD */
  absolute_address_t tmp;


  op = bus_read8_abs (pc++);
  
  if (op == 0x10) /* prefix for PAGE2 opcodes */
    {
      op = bus_read8_abs (pc++);
      am = codes10[op].mode;
      op = codes10[op].code;
    }
  else if (op == 0x11) /* prefix for PAGE3 opcodes */
    {
      op = bus_read8_abs (pc++);
      am = codes11[op].mode;
      op = codes11[op].code;
    }
  else
    {
      am = codes[op].mode;
      op = codes[op].code;
    }

  op_str = mne[op];
  if ((!strcmp("SWI2", op_str)) && sim_get_os9call())
    {
      op = bus_read8_abs (pc++);
      if(op < 0x91)
        {
          buf += sprintf (buf, "%-6.6s%s", "OS9", os9syscall[op]);
        }
      else
        {
          buf += sprintf (buf, "%-6.6s#$%2x", "OS9", op);
        }
    }
  else
    {
      buf += sprintf (buf, "%-6.6s", op_str);
    }
  switch (am)
    {
    case _illegal:
      sprintf (buf, "???");
      break;
    case _implied:
      break;
    case _imm_byte:
      sprintf (buf, "#$%02X", bus_read8_abs (pc++));
      break;
    case _imm_word:
      pc += 2;
      sprintf (buf, "#$%04X", bus_read16_abs(pc-2));
      break;
    case _direct:
      sprintf (buf, "<%s", monitor_addr_name (bus_read8_abs (pc++)));
      break;
    case _extended:
      pc += 2;
      sprintf (buf, "%s", monitor_addr_name (bus_read16_abs(pc-2)));
      break;

    case _indexed:
      op = bus_read8_abs (pc++);
      R = index_reg[(op >> 5) & 0x3];

      if ((op & 0x80) == 0)
	{
	  sprintf (buf, "%s,%c", off4[op & 0x1f], R);
	  break;
	}

      switch (op & 0x1f)
	{
	case 0x00:
	  sprintf (buf, ",%c+", R);
	  break;
	case 0x01:
	  sprintf (buf, ",%c++", R);
	  break;
	case 0x02:
	  sprintf (buf, ",-%c", R);
	  break;
	case 0x03:
	  sprintf (buf, ",--%c", R);
	  break;
	case 0x04:
	  sprintf (buf, ",%c", R);
	  break;
	case 0x05:
	  sprintf (buf, "B,%c", R);
	  break;
	case 0x06:
	  sprintf (buf, "A,%c", R);
	  break;
	case 0x08:
	  sprintf (buf, "$%02X,%c", bus_read8_abs (pc++), R);
	  break;
	case 0x09:
    pc += 2;
	  sprintf (buf, "$%04X,%c", bus_read16_abs(pc-2), R);
	  break;
	case 0x0B:
	  sprintf (buf, "D,%c", R);
	  break;
	case 0x0C:
	  sprintf (buf, "$%02X,PC", bus_read8_abs (pc++));
	  break;
	case 0x0D:
    pc += 2;
	  sprintf (buf, "$%04X,PC", bus_read16_abs(pc-2));
	  break;
	case 0x11:
	  sprintf (buf, "[,%c++]", R);
	  break;
	case 0x13:
	  sprintf (buf, "[,--%c]", R);
	  break;
	case 0x14:
	  sprintf (buf, "[,%c]", R);
	  break;
	case 0x15:
	  sprintf (buf, "[B,%c]", R);
	  break;
	case 0x16:
	  sprintf (buf, "[A,%c]", R);
	  break;
	case 0x18:
	  sprintf (buf, "[$%02X,%c]", bus_read8_abs (pc++), R);
	  break;
	case 0x19:
    pc += 2;
	  sprintf (buf, "[$%04X,%c]", bus_read16_abs(pc-2), R);
	  break;
	case 0x1B:
	  sprintf (buf, "[D,%c]", R);
	  break;
	case 0x1C:
	  sprintf (buf, "[$%02X,PC]", bus_read8_abs (pc++));
	  break;
	case 0x1D:
    pc += 2; 
	  sprintf (buf, "[$%04X,PC]", bus_read16_abs(pc-2));
	  break;
	case 0x1F:
    pc += 2;
	  sprintf (buf, "[%s]", monitor_addr_name (bus_read16_abs(pc-2)));
	  break;
	default:
	  sprintf (buf, "???");
	  break;
	}
      break;

    case _rel_byte:
           fetch1 = ((INT8) bus_read8_abs (pc++));
	   sprintf (buf, "%s", absolute_addr_name (fetch1 + pc));
      break;

    case _rel_word:
          pc +=2;
           tmp = bus_read16_abs(pc-2);
           sprintf (buf, "%s", absolute_addr_name (pc + tmp));
      break;

    case _reg_post:
      op = bus_read8_abs (pc++);
      sprintf (buf, "%s,%s", reg[op >> 4], reg[op & 15]);
      break;

    case _usr_post:
    case _sys_post:
      op = bus_read8_abs (pc++);

      if (op & 0x80)
	strcat (buf, "PC,");
      if (op & 0x40)
	strcat (buf, am == _usr_post ? "S," : "U,");
      if (op & 0x20)
	strcat (buf, "Y,");
      if (op & 0x10)
	strcat (buf, "X,");
      if (op & 0x08)
	strcat (buf, "DP,");
      if ((op & 0x06) == 0x06)
	strcat (buf, "D,");
      else
	{
	  if (op & 0x04)
	    strcat (buf, "B,");
	  if (op & 0x02)
	    strcat (buf, "A,");
	}
      if (op & 0x01)
	strcat (buf, "CC,");
      buf[strlen (buf) - 1] = '\0';
      break;

    }
  return pc - opc;
}

// nac it would be nice to have an intelligent map file reader..
int monitor_load_map_file (const char *name)
{
	FILE *fp;
	char map_filename[256];
	char buf[256];
	char *tok_ptr, *value_ptr, *id_ptr;
	target_addr_t value;

	/* Try appending the suffix 'map' to the name of the program. */
	sprintf (map_filename, "%s.map", name);
	fp = file_open (NULL, map_filename, "r");
	if (!fp)
	{
		/* If that fails, try replacing any existing suffix. */
		sprintf (map_filename, "%s", name);
		char *s = strrchr (map_filename, '.');
		if (s)
		{
			sprintf (s+1, "map");
			fp = file_open(NULL, map_filename, "r");
		}

		if (!fp)
		{
			fprintf (stderr, "warning: no symbols for %s\n", name);
			return -1;
		}
	}

	printf ("(dbg) Reading symbols from '%s'...\n", map_filename);
	for (;;)
	{
		fgets (buf, sizeof(buf)-1, fp);
		if (feof (fp))
			break;

                tok_ptr = strtok (buf, " \t\n");
                if (0 != strcmp(tok_ptr, "Symbol:"))
                    continue;

                id_ptr =  strtok(NULL, " \t\n");
                // skip over filename
                tok_ptr = strtok (NULL, " \t\n");
                // skip over "="
                tok_ptr = strtok (NULL, " \t\n");
                value_ptr = strtok (NULL, " \t\n");
                // get value as hex string
                value = (target_addr_t) strtoul(value_ptr, NULL, 16);

		sym_add (PROGRAM_SYMTAB_T, id_ptr, to_absolute (value), 0);
	}

	fclose (fp);
	return 0;
}

int load_hex (FILE *fp)
{
  unsigned int count, addr, type, data, checksum;
  int done = 1;
  int line = 0;

  while (done != 0)
    {
      line++;

      if (fscanf (fp, ":%2x%4x%2x", &count, &addr, &type) != 3)
	{
	  printf ("line %d: invalid hex record information.\n", line);
	  break;
	}
      checksum = count + (addr >> 8) + (addr & 0xff) + type;

      switch (type)
	{
	case 0:
	  for (; count != 0; count--, addr++, checksum += data)
	    {
              if (fscanf(fp, "%2x", &data))
                {
		   bus_write8(addr, (UINT8) data);
                }
              else
                {
                  printf("line %d: hex record data inconsistent with count field.\n", line);
	          break;
                }
	    }

	  checksum = (-checksum) & 0xff;

          if ( (fscanf(fp, "%2x", &data) != 1) || (data != checksum) )
	    {
	      printf("line %d: hex record checksum missing or invalid.\n", line);
	      done = 0;
	      break;
	    }
          fscanf(fp, "%*[\r\n]"); /* skip any form of line ending */
	  break;

	case 1:
	  checksum = (-checksum) & 0xff;

          if ( (fscanf(fp, "%2x", &data) != 1) || (data != checksum) )
	    printf("line %d: hex record checksum missing or invalid.\n", line);
	  done = 0;
	  break;

	case 2:
	default:
	  printf("line %d: not supported hex type %d.\n", line, type);
	  done = 0;
	  break;
	}
    }

  (void) fclose (fp);
  return 0;
}

int load_s19(FILE *fp)
{
  unsigned int count, addr, type, data, checksum;
  int done = 1;
  int line = 0;

  while (done != 0)
    {
      line++;

      if (fscanf(fp, "S%1x%2x%4x", &type, &count, &addr) != 3)
	{
	  printf("line %d: invalid S record information.\n", line);
	  break;
	}

      checksum = count + (addr >> 8) + (addr & 0xff);

      switch (type)
	{
	case 0:
	case 1:
	case 5:
	  for (count -= 3; count != 0; count--, addr++, checksum += data)
	    {
               if (fscanf (fp, "%2x", &data))
                  {
							if(type == 1)
								bus_write8 (addr, (UINT8) data);
                  }
               else
                  {
                    printf ("line %d: S record data inconsistent with count field.\n", line);
	            break;
                  }
	    }

	  checksum = (~checksum) & 0xff;

	  if ( (fscanf (fp, "%2x", &data) != 1) || (data != checksum) )
	    {
	      printf ("line %d: S record checksum missing or invalid.\n", line);
	      done = 0;
	      break;
	    }
          fscanf (fp, "%*[\r\n]"); /* skip any form of line ending */
	  break;

	case 9:
	  checksum = (~checksum) & 0xff;
	  if ( (fscanf (fp, "%2x", &data) != 1) || (data != checksum) )
	    printf ("line %d: S record checksum missing or invalid.\n", line);
	  done = 0;
	  break;

	default:
	  printf ("line %d: S%d not supported.\n", line, type);
	  done = 0;
	  break;
	}
    }

  (void) fclose(fp);
  return 0;
}

/* Auto-detect image file type and load it. For this to work,
   the machine must already be initialized.
*/
int monitor_load_image (const char *name)
{
  unsigned int count, addr, type;
  FILE *fp;

  fp = file_open(NULL, name, "r");
  if (fp == NULL)
    {
      printf("failed to open image file %s.\n", name);
      return 1;
    }

  if (fscanf (fp, "S%1x%2x%4x", &type, &count, &addr) == 3)
    {
        rewind(fp);
        return load_s19(fp);
    }
  else if (fscanf (fp, ":%2x%4x%2x", &count, &addr, &type) == 3)
    {
        rewind(fp);
        return load_hex(fp);
    }
  else
    {
      printf ("unrecognised format in image file %s.\n", name);
        return 1;
    }
}

void monitor_call (unsigned int flags)
{
   (void) flags; // FIXME: this variable is never used
#ifdef CALL_STACK
	if (current_function_call <= &fctab[MAX_FUNCTION_CALLS-1])
	{
		current_function_call++;
		current_function_call->entry_point = m6809_get_pc ();
		current_function_call->flags = flags;
	}
#endif
#if 0
	const char *id = sym_lookup (&program_symtab, to_absolute (m6809_get_pc ()));
	if (id)
	{
		// printf ("In %s now\n", id);
	}
#endif
}

void monitor_return (void)
{
#ifdef CALL_STACK
	if (current_function_call > &fctab[MAX_FUNCTION_CALLS-1])
	{
		current_function_call--;
		return;
	}

	while ((current_function_call->flags & FC_TAIL_CALL) &&
		(current_function_call > fctab))
	{
		current_function_call--;
	}

	if (current_function_call > fctab)
		current_function_call--;
#endif
}

const char* monitor_addr_name (target_addr_t target_addr)
{
   static char buf[256], *bufptr;
   const char *name;
   absolute_address_t addr = to_absolute (target_addr);

   bufptr = buf;

   bufptr += sprintf (bufptr, "$%04X", target_addr);

   name = sym_lookup (PROGRAM_SYMTAB_T, addr);
   if (name)
      sprintf (bufptr, "  <%s>", name);

   return buf;
}

static void monitor_signal (int sigtype)
{
   (void) sigtype;
   putchar ('\n');
   monitor_set_debug(ACTIVATED);
}

void monitor_init (void)
{
  BOOLEAN bool;
	fctab[0].entry_point = bus_read16 (0xfffe);
	memset (&fctab[0].entry_regs, 0, sizeof (struct cpu_regs));
	current_function_call = &fctab[0];
	auto_break_insn_count = 0;
	signal (SIGINT, monitor_signal);
	bool = sim_get_debug_status();
	monitor_set_debug(bool);
}

int check_break (void)
{
	if (dump_every_insn)
		print_current_insn ();

	if (auto_break_insn_count > 0)
		if (--auto_break_insn_count == 0)
			return 1;
	return 0;
}

void monitor_backtrace (void)
{
	struct function_call *fc = current_function_call;
	while (fc >= &fctab[0]) {
		printf ("%s\n", monitor_addr_name (fc->entry_point));
		fc--;
	}
}


/*
Monitor entry point
*/
int monitor6809 (void)
{
	int rc;
	rc = 0;
	signal (SIGINT, monitor_signal);
	rc = command_loop ();
  monitor_set_debug(FALSE);
	return rc;
}
