
#ifndef __CPU_6502_H
#define __CPU_6502_H


namespace cpu
{

//#define GNUC
#ifdef GNUC
#	define INLINE __attribute__((always_inline))
#elif MSVC
#	define INLINE __forceinline
#else
#	define INLINE inline
#endif


enum
{
	CPU_NMI_VECTOR = 0xFFFA,
	CPU_RESET_VECTOR = 0xFFFC,
	CPU_IRQ_VECTOR = 0xFFFE
};

typedef union
{
	struct {
		unsigned char LO;
		unsigned char HI;
	};
	unsigned short W;
} pair_t;

typedef union
{
	struct {
		unsigned C : 1;
		unsigned Z : 1;
		unsigned I : 1;
		unsigned D : 1;
		unsigned B : 1;
		unsigned   : 1;
		unsigned V : 1;
		unsigned N : 1;
	};
	unsigned char value;
} cpu_status_t;


extern unsigned char A;
extern unsigned char X;
extern unsigned char Y;
extern unsigned char S;
extern pair_t PC;
extern pair_t Addr;
extern cpu_status_t ST;
extern int cycles;
extern int cycles_elapsed;
extern unsigned char irq_line;
extern unsigned char reset_line;
extern unsigned char nmi_line;
extern unsigned long long cycles_master;
extern int frame;

int Run (int amount);
void Reset ();
void Debug();
void Init();
void IRQ();
void NMI();
void Frame();
int Dasm(unsigned short _pc, char *dasm_buffer);

#define __  0x00
#define ZF 0x02
#define NF 0x80
static const unsigned char zn_table[256] =
{
/*00*/	ZF, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
/*10*/  __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
/*20*/  __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
/*30*/  __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
/*40*/  __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
/*50*/  __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
/*60*/  __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
/*70*/  __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
/*80*/  NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF,
/*90*/  NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF,
/*A0*/  NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF,
/*B0*/  NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF,
/*C0*/  NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF,
/*D0*/  NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF,
/*E0*/  NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF,
/*F0*/  NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF, NF,
};
#undef NF
#undef ZF
#undef __

}

#endif/*__CPU_6502_H*/
