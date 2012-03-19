/* M6502 (NES VERSION) core
 *
 * NO DECIMAL MODE
 *
 * Marcos Medeiros 
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include "cpu.h"



namespace bus
{
INLINE unsigned char Read(int a)
{
//	return bus_read(a);
	return 0;
}
INLINE void Write(unsigned a, unsigned char data)
{
//	bus_write(a, data);
}
}

namespace cpu
{
int frame = 0;
/* General registers */
unsigned char A;
unsigned char X;
unsigned char Y;
/* Status register */
cpu_status_t ST;
/* Stack pointer */
unsigned char S;
/* Program counter */
pair_t PC;
/* Effective address */
pair_t Addr;
/* Temp data and opcode */
unsigned char data = 0;
unsigned char opcode = 0;
unsigned char page_crossed = 0;
unsigned char irq_line = 0;
unsigned char reset_line = 0;
unsigned char nmi_line = 0;
/* Cycles elapsed */
int cycles = 0; /* current Run(n) call */
int cycles_elapsed = 0;
unsigned long long cycles_master = 0ULL;


static const unsigned char cycle_table[256] = {
	7, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
	0, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
	0, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	6, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
	0, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
	0, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
	0, 6, 0, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
	0, 5, 0, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
	0, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
	0, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
};

/* This macro define a function like AddrThis() */
#define CPU_ADDR_DEF(name)	INLINE void Addr ##name()
#define CPU_READ_DEF(name)	INLINE void Read ##name() { Addr ##name(); data = bus::Read(Addr.W); }
#define CPU_WRITE_DEF(name)	INLINE void Write ##name(unsigned char val) { Addr ##name(); bus::Write(Addr.W, val); }
#define CPU_OPCODE_DEF(name) INLINE void ##name()

void Init()
{
}

/*
 * Addressing Modes
 */
CPU_ADDR_DEF(Immediate)
{
	Addr.W = PC.W++;
}

CPU_ADDR_DEF(ZeroPage)
{
	Addr.W = bus::Read (PC.W++);
}

CPU_ADDR_DEF(ZeroPageX)
{
	AddrZeroPage();
	Addr.LO += X;
	Addr.W &= 0xFF;
}
CPU_ADDR_DEF(ZeroPageY)
{
	AddrZeroPage();
	Addr.LO += Y;
	Addr.W &= 0xFF;
}

CPU_ADDR_DEF(Absolute)
{
	Addr.LO = bus::Read (PC.W++);
	Addr.HI = bus::Read (PC.W++);
}

CPU_ADDR_DEF(AbsoluteX)
{
	AddrAbsolute();
	Addr.W += X;
}

CPU_ADDR_DEF(AbsoluteY)
{
	AddrAbsolute();
	Addr.W += Y;
}

CPU_ADDR_DEF(Indirect)
{
    Addr.LO = bus::Read (PC.W++);
	Addr.HI = bus::Read (PC.W++);
    unsigned short tmp = Addr.W;
	Addr.LO = bus::Read (tmp++);
	if (!(tmp & 0xFF))
		tmp -= 0x100;
	Addr.HI = bus::Read (tmp);
}

CPU_ADDR_DEF(IndirectX)
{
	AddrZeroPage();
	Addr.W += X;
	Addr.W &= 0xFF;
	unsigned short tmp = Addr.W;
	Addr.LO = bus::Read (tmp++);
	tmp &= 0xFF;
	Addr.HI = bus::Read (tmp);
}

CPU_ADDR_DEF(IndirectY)
{
	AddrZeroPage();
	Addr.W &= 0xFF;
	unsigned short tmp = Addr.W;
	Addr.LO = bus::Read (tmp++);
	tmp &= 0xFF;
	Addr.HI = bus::Read (tmp);

	Addr.W += Y;
}

CPU_ADDR_DEF(Relative)
{
    signed char displace = bus::Read(PC.W++);

    Addr.W = (unsigned short)((signed short) PC.W + displace);
}

/*
 * Read Addressing Modes
 */


CPU_READ_DEF(Immediate);
CPU_READ_DEF(ZeroPage);
CPU_READ_DEF(ZeroPageX);
CPU_READ_DEF(ZeroPageY);
CPU_READ_DEF(Absolute);
CPU_READ_DEF(AbsoluteX);
CPU_READ_DEF(AbsoluteY);
CPU_READ_DEF(IndirectX);
CPU_READ_DEF(IndirectY);

/*
 * recalculate Zero and Negative flags
 */
INLINE void ZN(unsigned char value)
{
	ST.value &= ~0x82;
	ST.value |= zn_table[value];
}


INLINE void PUSH(unsigned char value)
{

	bus::Write(0x100 | S, value);
	--S;
}

INLINE unsigned char POP()
{
	++S;
	return bus::Read(0x100 | S);
}

/*
 * OPCODES
 */

INLINE void RTI()
{
	ST.value = POP() & ~0x10;
	ST.value |= 0x20;
	PC.LO = POP();
	PC.HI = POP();
}



INLINE void BIT()
{
	ST.value &= ~0xC2;
	if (!(A & data))
		ST.Z = 1;

	ST.value |= (data & 0xC0);
}

INLINE void AND()
{
	A &= data;
	ZN (A);
}

INLINE void ORA()
{
	A |= data;
	ZN (A);
}

INLINE void EOR()
{
	A ^= data;
	ZN (A);
}

INLINE void CMP()
{
	unsigned char tmp = A - data;
	ST.value &= ~0x83;
	if (A >= data)
		ST.C = 1;
	ZN (tmp);
}

INLINE void CPX()
{
	unsigned char tmp = X - data;
	ST.value &= ~0x83;
	if (X >= data)
		ST.C = 1;
	ZN (tmp);
}

INLINE void CPY()
{
	unsigned char tmp = Y - data;
	ST.value &= ~0x83;
	if (Y >= data)
		ST.C = 1;
	ZN (tmp);
}



INLINE void ADC()
{
	unsigned short sum = A + data + (ST.C ? 1 : 0);

	/* positive + positive = negative */
	if ((A ^ sum) & (data ^ sum) & 0x80)
		ST.V = 1;
	else
		ST.V = 0;
	if (sum >= 0x100)
		ST.C = 1;
	else
		ST.C = 0;

	A = sum;
	ZN (A);
}

INLINE void SBC()
{
	data ^= 0xFF;
	ADC();
}

INLINE void LSRA()
{
	ST.C = (A & 1);
	A >>= 1;
	ZN (A);
}

INLINE void LSR()
{
	ST.C = (data & 1);
	data >>= 1;
	ZN (data);
	bus::Write (Addr.W, data);
}

INLINE void ASLA()
{
	ST.C = (A & 0x80) ? 1 : 0;
	A <<= 1;
	ZN (A);
}

INLINE void ASL()
{
	ST.C = (data & 0x80) ? 1 : 0;
	data <<= 1;
	ZN (data);
	bus::Write (Addr.W, data);
}


INLINE void ROLA()
{
	unsigned char bit = (ST.C ? 1 : 0);
	ST.C = (A & 0x80) ? 1 : 0;
	A <<= 1;
	A |= bit;
	ZN (A);
}

INLINE void ROL()
{
	unsigned char bit = (ST.C ? 1 : 0);
	ST.C = (data & 0x80) ? 1 : 0;
	data <<= 1;
	data |= bit;
	ZN (data);
	bus::Write (Addr.W, data);
}

INLINE void RORA()
{
	unsigned char bit = (ST.C ? 0x80 : 0);
	ST.C = (A & 0x1) ? 1 : 0;
	A >>= 1;
	A |= bit;
	ZN (A);
}

INLINE void ROR()
{
	unsigned char bit = (ST.C ? 0x80 : 0);
	ST.C = (data & 0x1) ? 1 : 0;
	data >>= 1;
	data |= bit;
	ZN (data);
	bus::Write (Addr.W, data);
}

INLINE void INC()
{
	++data;
	ZN (data);
	bus::Write (Addr.W, data);
}

INLINE void DEC()
{
	--data;
	ZN (data);
	bus::Write (Addr.W, data);
}

INLINE void JSR()
{
	--PC.W;
	PUSH(PC.HI);
	PUSH(PC.LO);
	PC.W = Addr.W;
}

INLINE void PLA()
{
	A = POP();
	ZN (A);
}

INLINE void PLP()
{
	ST.value = POP() & ~0x10;
	ST.value |= 0x20;
}

INLINE void RTS()
{
	PC.LO = POP();
	PC.HI = POP();
	++PC.W;
}

INLINE void JMP()
{
	PC.W = Addr.W;
}


INLINE void LDA()
{
	A = data;
	ZN (A);
}

INLINE void LDX()
{
	X = data;
	ZN (X);
}

INLINE void LDY()
{
	Y = data;
	ZN (Y);
}

INLINE void INX()
{
	++X;
	ZN (X);
}

INLINE void INY()
{
	++Y;
	ZN (Y);
}

INLINE void DEX()
{
	--X;
	ZN (X);
}

INLINE void DEY()
{
	--Y;
	ZN (Y);
}

INLINE void TAX()
{
	X = A;
	ZN (X);
}

INLINE void TAY()
{
	Y = A;
	ZN (Y);
}

INLINE void TSX()
{
	X = S;
	ZN (X);
}

INLINE void TXA()
{
	A = X;
	ZN (A);
}

INLINE void TXS()
{
	S = X;
}

INLINE void TYA()
{
	A = Y;
	ZN (A);
}

INLINE void NOP()
{
	/* :-) */
}

INLINE void CLC()
{
	ST.C = 0;
}

INLINE void CLD()
{
	ST.D = 0;
}

INLINE void CLI()
{
	/*
	 * I'm sure that it does more than this... :-)
	 * but for now, it's what really matters...
	 */
	ST.I = 0;
}

INLINE void CLV()
{
	ST.V = 0;
}

INLINE void SEC()
{
	ST.C = 1;
}
INLINE void SED()
{
	ST.D = 1;
}
INLINE void SEI()
{
	ST.I = 1;
}

INLINE void PHA()
{
	PUSH(A);
}

INLINE void PHP()
{
	/* We need to mask bits before push status to stack */
	PUSH(ST.value | 0x30);
}

INLINE void STA()
{
	bus::Write (Addr.W, A);
}

INLINE void STX()
{
	bus::Write (Addr.W, X);
}

INLINE void STY()
{
	bus::Write (Addr.W, Y);
}



INLINE void BCC()
{
	if (!ST.C) {
		PC.W = Addr.W;
		cycles += 3;
	} else
		cycles += 2;
}
INLINE void BCS()
{
	if (ST.C) {
		PC.W = Addr.W;
		cycles += 3;
	} else
		cycles += 2;
}
INLINE void BEQ()
{
	if (ST.Z) {
		PC.W = Addr.W;
		cycles += 3;
	} else
		cycles += 2;
}
INLINE void BMI()
{
	if (ST.N) {
		PC.W = Addr.W;
		cycles += 3;
	} else
		cycles += 2;
}
INLINE void BNE()
{
	if (!ST.Z) {
		PC.W = Addr.W;
		cycles += 3;
	} else
		cycles += 2;
}
INLINE void BPL()
{
	if (!ST.N) {
		PC.W = Addr.W;
		cycles += 3;
	} else
		cycles += 2;
}
INLINE void BVC()
{
	if (!ST.V) {
		PC.W = Addr.W;
		cycles += 3;
	} else
		cycles += 2;
}
INLINE void BVS()
{
	if (ST.V) {
		PC.W = Addr.W;
		cycles += 3;
	} else
		cycles += 2;
}

INLINE void BRK()
{
	PUSH(PC.HI);
	PUSH(PC.LO+1);
	PHP();
	SEI();
	PC.LO = bus::Read (CPU_IRQ_VECTOR);
	PC.HI = bus::Read (CPU_IRQ_VECTOR + 1);
}

/* UNDOCUMENTED OPCODES */
INLINE void LAX()
{
	X = A = data;
	ZN (A);
}

INLINE void SAX()
{
	bus::Write(Addr.W, X & A);
}


INLINE void DCP()
{
	--data;
	bus::Write(Addr.W, data);
	CMP();
}

INLINE void ISC()
{
	++data;
	bus::Write(Addr.W, data);
	SBC();
}

INLINE void SLO()
{
	ASL();
	ORA();
}

INLINE void RLA()
{
	ROL();
	AND();
}

INLINE void SRE()
{
	LSR();
	EOR();
}

INLINE void RRA()
{
	ROR();
	ADC();
}


FILE *log = NULL;

void Trace()
{
    if (!log)
	{
	    log = fopen("trace.txt","w");
	    if (!log)
            exit(6);
	}
	static char dasm_buffer[32];
	static char raw_buffer[10];
	int op_size = Dasm(PC.W, dasm_buffer);

	fprintf(log, "%04X  ", PC.W);

	switch (op_size)
	{
	    case 1:
            sprintf(raw_buffer, "%02X      ", bus::Read(PC.W));
            break;
        case 2:
            sprintf(raw_buffer, "%02X %02X   ", bus::Read(PC.W), bus::Read(PC.W+1));
            break;
        case 3:
            sprintf(raw_buffer, "%02X %02X %02X", bus::Read(PC.W),
                    bus::Read(PC.W+1), bus::Read(PC.W+2));
            break;
	}

    fprintf(log, "%s  ", raw_buffer);
    fprintf(log, "%s", dasm_buffer);

    int pad = 32 - strlen(dasm_buffer);
    while (pad--) fprintf(log, " ");

    fprintf(log, "A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
            A, X, Y, ST.value, S);
}

void TraceToConsole()
{
	static char dasm_buffer[32];
	static char raw_buffer[10];
	int op_size = Dasm(PC.W, dasm_buffer);

	fprintf(stdout, "%04X | ", PC.W);

	switch (op_size)
	{
	    case 1:
            sprintf(raw_buffer, "%02X       |", bus::Read(PC.W));
            break;
        case 2:
            sprintf(raw_buffer, "%02X %02X    |", bus::Read(PC.W), bus::Read(PC.W+1));
            break;
        case 3:
            sprintf(raw_buffer, "%02X %02X %02X |", bus::Read(PC.W),
                    bus::Read(PC.W+1), bus::Read(PC.W+2));
            break;
	}

    fprintf(stdout, "%s  ", raw_buffer);
    fprintf(stdout, "%s", dasm_buffer);

    int pad = 16 - strlen(dasm_buffer);
    while (pad--) fprintf(stdout, " ");
    fprintf(stdout,"| ");

    fprintf(stdout, "A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
            A, X, Y, ST.value, S);
    fflush(stdout);
}

/*
 *
 */
int Run (int amount)
{
    cycles = 0;
	
	while (cycles <= amount)
	{


		if (nmi_line)
		{
			PUSH(PC.HI);
			PUSH(PC.LO);
			PUSH(ST.value);
			PC.LO = bus::Read (CPU_NMI_VECTOR);
			PC.HI = bus::Read (CPU_NMI_VECTOR + 1);
			cycles += 7;
			nmi_line = 0;
			continue;
		}
		/* check for interrupt lines */
		if ((!ST.I) && irq_line)
		{
			PUSH(PC.HI);
			PUSH(PC.LO);
			PUSH(ST.value);
			ST.I = 1;
			PC.LO = bus::Read (CPU_IRQ_VECTOR);
			PC.HI = bus::Read (CPU_IRQ_VECTOR + 1);
			cycles += 7;
			irq_line = 0;
			continue;
		}


		opcode = bus::Read (PC.W);
		//opcode = bus_fetch(PC.W);
		//Trace();
		page_crossed = 0;
		++PC.W;

		switch (opcode)
		{
		/*
		 * ROR A
		 * ROR $zp
		 * ROR $zp, x
		 * ROR $addr
		 * ROR $addr, x
		 */
		case 0x6A:						RORA();	break;
		case 0x66: ReadZeroPage();		ROR();	break;
		case 0x76: ReadZeroPageX();		ROR();	break;
		case 0x6E: ReadAbsolute();		ROR();	break;
		case 0x7E: ReadAbsoluteX();		ROR();	break;
		/*
		 * ROL A
		 * ROL $zp
		 * ROL $zp, x
		 * ROL $addr
		 * ROL $addr, x
		 */
		case 0x2A:						ROLA();	break;
		case 0x26: ReadZeroPage();		ROL();	break;
		case 0x36: ReadZeroPageX();		ROL();	break;
		case 0x2E: ReadAbsolute();		ROL();	break;
		case 0x3E: ReadAbsoluteX();		ROL();	break;



		/*
		 * LSR A
		 * LSR $zp
		 * LSR $zp, x
		 * LSR $addr
		 * LSR $addr, x
		 */
		case 0x4A:						LSRA();	break;
		case 0x46: ReadZeroPage();		LSR();	break;
		case 0x56: ReadZeroPageX();		LSR();	break;
		case 0x4E: ReadAbsolute();		LSR();	break;
		case 0x5E: ReadAbsoluteX();		LSR();	break;
		/*
		 * ASL A
		 * ASL $zp
		 * ASL $zp, x
		 * ASL $addr
		 * ASL $addr, x
		 */
		case 0x0A:						ASLA();	break;
		case 0x06: ReadZeroPage();		ASL();	break;
		case 0x16: ReadZeroPageX();		ASL();	break;
		case 0x0E: ReadAbsolute();		ASL();	break;
		case 0x1E: ReadAbsoluteX();		ASL();	break;
		/*
		 * INC $zp
		 * INC $zp, x
		 * INC $addr
		 * INC $addr, x
		 */
		case 0xE6: ReadZeroPage();		INC(); break;
		case 0xF6: ReadZeroPageX();		INC(); break;
		case 0xEE: ReadAbsolute();		INC(); break;
		case 0xFE: ReadAbsoluteX();		INC(); break;
		/*
		 * DEC $zp
		 * DEC $zp, x
		 * DEC $addr
		 * DEC $addr, x
		 */
		case 0xC6: ReadZeroPage();		DEC(); break;
		case 0xD6: ReadZeroPageX();		DEC(); break;
		case 0xCE: ReadAbsolute();		DEC(); break;
		case 0xDE: ReadAbsoluteX();		DEC(); break;

		/*
		 * CPX #imm
		 * CPX $zp
		 * CPX $addr
		 */
		case 0xE0: ReadImmediate();		CPX(); break;
		case 0xE4: ReadZeroPage();		CPX(); break;
		case 0xEC: ReadAbsolute();		CPX(); break;

		/*
		 * CPY #imm
		 * CPY $zp
		 * CPY $addr
		 */
		case 0xC0: ReadImmediate();		CPY(); break;
		case 0xC4: ReadZeroPage();		CPY(); break;
		case 0xCC: ReadAbsolute();		CPY(); break;

		/*
		 * ADC #imm
		 * ADC $zp
		 * ADC $zp, x
		 * ADC $addr
		 * ADC $addr, x
		 * ADC $addr, y
		 * ADC ($zp, x)
		 * ADC ($zp), y
		 */
		case 0x69: ReadImmediate();		ADC();	break;
		case 0x65: ReadZeroPage();		ADC();	break;
		case 0x75: ReadZeroPageX();		ADC();	break;
		case 0x6D: ReadAbsolute();		ADC();	break;
		case 0x7D: ReadAbsoluteX();		ADC();	break;
		case 0x79: ReadAbsoluteY();		ADC();	break;
		case 0x61: ReadIndirectX();		ADC();	break;
		case 0x71: ReadIndirectY();		ADC();	break;
		/*
		 * SBC #imm
		 * SBC $zp
		 * SBC $zp, x
		 * SBC $addr
		 * SBC $addr, x
		 * SBC $addr, y
		 * SBC ($zp, x)
		 * SBC ($zp), y
		 */
		case 0xE9: ReadImmediate();		SBC();	break;
		case 0xE5: ReadZeroPage();		SBC();	break;
		case 0xF5: ReadZeroPageX();		SBC();	break;
		case 0xED: ReadAbsolute();		SBC();	break;
		case 0xFD: ReadAbsoluteX();		SBC();	break;
		case 0xF9: ReadAbsoluteY();		SBC();	break;
		case 0xE1: ReadIndirectX();		SBC();	break;
		case 0xF1: ReadIndirectY();		SBC();	break;
		/*
		 * EOR #imm
		 * EOR $zp
		 * EOR $zp, x
		 * EOR $addr
		 * EOR $addr, x
		 * EOR $addr, y
		 * EOR ($zp, x)
		 * EOR ($zp), y
		 */
		case 0x49: ReadImmediate();		EOR();	break;
		case 0x45: ReadZeroPage();		EOR();	break;
		case 0x55: ReadZeroPageX();		EOR();	break;
		case 0x4D: ReadAbsolute();		EOR();	break;
		case 0x5D: ReadAbsoluteX();		EOR();	break;
		case 0x59: ReadAbsoluteY();		EOR();	break;
		case 0x41: ReadIndirectX();		EOR();	break;
		case 0x51: ReadIndirectY();		EOR();	break;
		/*
		 * ORA #imm
		 * ORA $zp
		 * ORA $zp, x
		 * ORA $addr
		 * ORA $addr, x
		 * ORA $addr, y
		 * ORA ($zp, x)
		 * ORA ($zp), y
		 */
		case 0x09: ReadImmediate();		ORA();	break;
		case 0x05: ReadZeroPage();		ORA();	break;
		case 0x15: ReadZeroPageX();		ORA();	break;
		case 0x0D: ReadAbsolute();		ORA();	break;
		case 0x1D: ReadAbsoluteX();		ORA();	break;
		case 0x19: ReadAbsoluteY();		ORA();	break;
		case 0x01: ReadIndirectX();		ORA();	break;
		case 0x11: ReadIndirectY();		ORA();	break;
		/*
		 * CMP #imm
		 * CMP $zp
		 * CMP $zp, x
		 * CMP $addr
		 * CMP $addr, x
		 * CMP $addr, y
		 * CMP ($zp, x)
		 * CMP ($zp), y
		 */
		case 0xC9: ReadImmediate();		CMP();	break;
		case 0xC5: ReadZeroPage();		CMP();	break;
		case 0xD5: ReadZeroPageX();		CMP();	break;
		case 0xCD: ReadAbsolute();		CMP();	break;
		case 0xDD: ReadAbsoluteX();		CMP();	break;
		case 0xD9: ReadAbsoluteY();		CMP();	break;
		case 0xC1: ReadIndirectX();		CMP();	break;
		case 0xD1: ReadIndirectY();		CMP();	break;
		/*
		 * AND #imm
		 * AND $zp
		 * AND $zp, x
		 * AND $addr
		 * AND $addr, x
		 * AND $addr, y
		 * AND ($zp, x)
		 * AND ($zp), y
		 */
		case 0x29: ReadImmediate();		AND();	break;
		case 0x25: ReadZeroPage();		AND();	break;
		case 0x35: ReadZeroPageX();		AND();	break;
		case 0x2D: ReadAbsolute();		AND();	break;
		case 0x3D: ReadAbsoluteX();		AND();	break;
		case 0x39: ReadAbsoluteY();		AND();	break;
		case 0x21: ReadIndirectX();		AND();	break;
		case 0x31: ReadIndirectY();		AND();	break;
		/*
		 * BIT $zp
		 * BIT $addr
		 */
		case 0x24: ReadZeroPage();		BIT();	break;
		case 0x2C: ReadAbsolute();		BIT();	break;
		/*
		 * BCC $disp
		 * BCS $disp
		 * BEQ $disp
		 * BMI $disp
		 * BNE $disp
		 * BPL $disp
		 * BVC $disp
		 * BVS $disp
		 */
		case 0x90: AddrRelative();		BCC();	break;
		case 0xB0: AddrRelative();		BCS();	break;
		case 0xF0: AddrRelative();		BEQ();	break;
		case 0x30: AddrRelative();		BMI();	break;
		case 0xD0: AddrRelative();		BNE();	break;
		case 0x10: AddrRelative();		BPL();	break;
		case 0x50: AddrRelative();		BVC();	break;
		case 0x70: AddrRelative();		BVS();	break;


		/*
		 * STA $zp
		 * STA $zp, x
		 * STA $addr
		 * STA $addr, x
		 * STA $addr, y
		 * STA ($zp, x)
		 * STA ($zp), y
		 */
		case 0x85: AddrZeroPage();		STA();	break;
		case 0x95: AddrZeroPageX();		STA();	break;
		case 0x8D: AddrAbsolute();		STA();	break;
		case 0x9D: AddrAbsoluteX();		STA();	break;
		case 0x99: AddrAbsoluteY();		STA();	break;
		case 0x81: AddrIndirectX();		STA();	break;
		case 0x91: AddrIndirectY();		STA();	break;
		/*
		 * STX $zp
		 * STX $zp, y
		 * STX $addr
		 */
		case 0x86: AddrZeroPage();		STX();	break;
		case 0x96: AddrZeroPageY();		STX();	break;
		case 0x8E: AddrAbsolute();		STX();	break;
		/*
		 * STY $zp
		 * STY $zp, x
		 * STY $addr
		 */
		case 0x84: AddrZeroPage();		STY();	break;
		case 0x94: AddrZeroPageX();		STY();	break;
		case 0x8C: AddrAbsolute();		STY();	break;
		/*
		 * PLA
		 * PLP
		 */
		case 0x68:						PLA();	break;
		case 0x28:						PLP();	break;
		/*
		 * PHA
		 * PHP
		 */
		case 0x48:						PHA();	break;
		case 0x08:						PHP();	break;
		/*
		 * SEC
		 * SED
		 * SEI
		 */
		case 0x38:						SEC();	break;
		case 0xF8:						SED();	break;
		case 0x78:						SEI();	break;
		/*
		 * CLC
		 * CLD
		 * CLI
		 * CLV
		 */
		case 0x18:						CLC();	break;
		case 0xD8:						CLD();	break;
		case 0x58:						CLI();	break;
		case 0xB8:						CLV();	break;
		/*
		 * NOP
		 */
		case 0xEA:						NOP();	break;
		/*
		 * RTI
		 * BRK
		 */
		case 0x40:						RTI();	break;
		case 0x00:						BRK();	break;
		/*
		 * INX
		 * INY
		 */
		case 0xE8:						INX();	break;
		case 0xC8:						INY();	break;
		/*
		 * DEX
		 * DEY
		 */
		case 0xCA:						DEX();	break;
		case 0x88:						DEY();	break;
		/*
		 * TAX
		 * TAY
		 * TSX
		 * TXA
		 * TXS
		 * TYA
		 */
		case 0xAA:						TAX();	break;
		case 0xA8:						TAY();	break;
		case 0xBA:						TSX();	break;
		case 0x8A:						TXA();	break;
		case 0x9A:						TXS();	break;
		case 0x98:						TYA();	break;
		/*
		 * JSR $addr
		 * RTS
		 */
		case 0x20: AddrAbsolute();		JSR();	break;
		case 0x60:						RTS();	break;
		/*
		 * JMP $addr
		 * JMP ($addr)
		 */
		case 0x4C: AddrAbsolute();		JMP();	break;
		case 0x6C: AddrIndirect();		JMP();	break;

		/*
		 * LDA #imm
		 * LDA $zp
		 * LDA $zp, x
		 * LDA $addr
		 * LDA $addr, x
		 * LDA $addr, y
		 * LDA ($zp, x)
		 * LDA ($zp), y
		 */
		case 0xA9: ReadImmediate();		LDA();	break;
		case 0xA5: ReadZeroPage();		LDA();	break;
		case 0xB5: ReadZeroPageX();		LDA();	break;
		case 0xAD: ReadAbsolute();		LDA();	break;
		case 0xBD: ReadAbsoluteX();		LDA();	break;
		case 0xB9: ReadAbsoluteY();		LDA();	break;
		case 0xA1: ReadIndirectX();		LDA();	break;
		case 0xB1: ReadIndirectY();		LDA();	break;
		/*
		 * LDX #imm
		 * LDX $zp
		 * LDX $zp, y
		 * LDX $addr
		 * LDX $addr, y
		 */
		case 0xA2: ReadImmediate();		LDX(); break;
		case 0xA6: ReadZeroPage();		LDX(); break;
		case 0xB6: ReadZeroPageY();		LDX(); break;
		case 0xAE: ReadAbsolute();		LDX(); break;
		case 0xBE: ReadAbsoluteY();		LDX(); break;
		/*
		 * LDY #imm
		 * LDY $zp
		 * LDY $zp, x
		 * LDY $addr
		 * LDY $addr, x
		 */
		case 0xA0: ReadImmediate();		LDY(); break;
		case 0xA4: ReadZeroPage();		LDY(); break;
		case 0xB4: ReadZeroPageX();		LDY(); break;
		case 0xAC: ReadAbsolute();		LDY(); break;
		case 0xBC: ReadAbsoluteX();		LDY(); break;

		/*
		 * UNDOCUMENTED OPCODES
		 */
		/*
		 * NOP
		 * NOP #$imm
		 * NOP $zp
		 * NOP $zp, x
		 * NOP $addr
		 * NOP $addr, x
		 */
		case 0x1A:
		case 0x3A:
		case 0x5A:
		case 0x7A:
		case 0xDA:
		case 0xFA:						NOP();	break;
		case 0x04:
		case 0x44:
		case 0x64: AddrZeroPage();		NOP();	break;
		case 0x14:
		case 0x34:
		case 0x54:
		case 0x74:
		case 0xD4:
		case 0xF4: AddrZeroPageX();		NOP();	break;
		case 0x0C: AddrAbsolute();		NOP();	break;
		case 0x80: AddrImmediate();		NOP();	break;
		case 0x1C:
		case 0x3C:
		case 0x5C:
		case 0x7C:
		case 0xDC:
		case 0xFC: AddrAbsoluteX();		NOP();	break;

		/*
		 * LAX $zp
		 * LAX $zp, y
		 * LAX $addr
		 * LAX $addr, y
		 * LAX ($zp, x)
		 * LAX ($zp), y
		 */
		case 0xA7: ReadZeroPage();		LAX();	break;
		case 0xB7: ReadZeroPageY();		LAX();	break;
		case 0xAF: ReadAbsolute();		LAX();	break;
		case 0xBF: ReadAbsoluteY();		LAX();	break;
		case 0xA3: ReadIndirectX();		LAX();	break;
		case 0xB3: ReadIndirectY();		LAX();	break;

		/*
		 * LAX $zp
		 * LAX $zp, y
		 * LAX $addr
		 * LAX ($zp, x)
		 */
		case 0x87: AddrZeroPage();		SAX();	break;
		case 0x97: ReadZeroPageY();		SAX();	break;
		case 0x8F: ReadAbsolute();		SAX();	break;
		case 0x83: ReadIndirectX();		SAX();	break;

		/*
		 * SBC #$imm
		 */
		case 0xEB: ReadImmediate();		SBC();	break;

		/*
		 * DCP $zp
		 * DCP $zp, x
		 * DCP $addr
		 * DCP $addr, x
		 * DCP $addr, y
		 * DCP ($zp, x)
		 * DCP ($zp), y
		 */
		case 0xC7: ReadZeroPage();		DCP();	break;
		case 0xD7: ReadZeroPageX();		DCP();	break;
		case 0xCF: ReadAbsolute();		DCP();	break;
		case 0xDF: ReadAbsoluteX();		DCP();	break;
		case 0xDB: ReadAbsoluteY();		DCP();	break;
		case 0xC3: ReadIndirectX();		DCP();	break;
		case 0xD3: ReadIndirectY();		DCP();	break;

		/*
		 * ISC $zp
		 * ISC $zp, x
		 * ISC $addr
		 * ISC $addr, x
		 * ISC $addr, y
		 * ISC ($zp, x)
		 * ISC ($zp), y
		 */
		case 0xE7: ReadZeroPage();		ISC();	break;
		case 0xF7: ReadZeroPageX();		ISC();	break;
		case 0xEF: ReadAbsolute();		ISC();	break;
		case 0xFF: ReadAbsoluteX();		ISC();	break;
		case 0xFB: ReadAbsoluteY();		ISC();	break;
		case 0xE3: ReadIndirectX();		ISC();	break;
		case 0xF3: ReadIndirectY();		ISC();	break;

		/*
		 * SLO $zp
		 * SLO $zp, x
		 * SLO $addr
		 * SLO $addr, x
		 * SLO $addr, y
		 * SLO ($zp, x)
		 * SLO ($zp), y
		 */
		case 0x07: ReadZeroPage();		SLO();	break;
		case 0x17: ReadZeroPageX();		SLO();	break;
		case 0x0F: ReadAbsolute();		SLO();	break;
		case 0x1F: ReadAbsoluteX();		SLO();	break;
		case 0x1B: ReadAbsoluteY();		SLO();	break;
		case 0x03: ReadIndirectX();		SLO();	break;
		case 0x13: ReadIndirectY();		SLO();	break;

		/*
		 * RLA $zp
		 * RLA $zp, x
		 * RLA $addr
		 * RLA $addr, x
		 * RLA $addr, y
		 * RLA ($zp, x)
		 * RLA ($zp), y
		 */
		case 0x27: ReadZeroPage();		RLA();	break;
		case 0x37: ReadZeroPageX();		RLA();	break;
		case 0x2F: ReadAbsolute();		RLA();	break;
		case 0x3F: ReadAbsoluteX();		RLA();	break;
		case 0x3B: ReadAbsoluteY();		RLA();	break;
		case 0x23: ReadIndirectX();		RLA();	break;
		case 0x33: ReadIndirectY();		RLA();	break;

		/*
		 * SRE $zp
		 * SRE $zp, x
		 * SRE $addr
		 * SRE $addr, x
		 * SRE $addr, y
		 * SRE ($zp, x)
		 * SRE ($zp), y
		 */
		case 0x47: ReadZeroPage();		SRE();	break;
		case 0x57: ReadZeroPageX();		SRE();	break;
		case 0x4F: ReadAbsolute();		SRE();	break;
		case 0x5F: ReadAbsoluteX();		SRE();	break;
		case 0x5B: ReadAbsoluteY();		SRE();	break;
		case 0x43: ReadIndirectX();		SRE();	break;
		case 0x53: ReadIndirectY();		SRE();	break;

		/*
		 * RRA $zp
		 * RRA $zp, x
		 * RRA $addr
		 * RRA $addr, x
		 * RRA $addr, y
		 * RRA ($zp, x)
		 * RRA ($zp), y
		 */
		case 0x67: ReadZeroPage();		RRA();	break;
		case 0x77: ReadZeroPageX();		RRA();	break;
		case 0x6F: ReadAbsolute();		RRA();	break;
		case 0x7F: ReadAbsoluteX();		RRA();	break;
		case 0x7B: ReadAbsoluteY();		RRA();	break;
		case 0x63: ReadIndirectX();		RRA();	break;
		case 0x73: ReadIndirectY();		RRA();	break;

		default:
			break;
		}

		cycles += cycle_table [opcode];

	}
	cycles_elapsed += cycles;
	/*if (cycles_elapsed >= ((341/3)*262))
	{
	    printf("CYC:%d burst %d\n", cycles_elapsed, ((341/3)*262));
	}*/
	return cycles - amount;
}


void Frame()
{
    ++frame;
	cycles_master += cycles_elapsed;
	cycles_elapsed = 0;
}



/*
 *
 */
void Reset ()
{
	A = 0;
	X = 0;
	Y = 0;
	S = 0xFD;
	ST.value = 0x24;
	PC.LO = bus::Read (CPU_RESET_VECTOR);
	PC.HI = bus::Read (CPU_RESET_VECTOR + 1);
	irq_line = 0;
	nmi_line = 0;
	reset_line = 0;
}

void IRQ()
{
	irq_line = 1;
}
void NMI()
{
	nmi_line = 1;
}


static const char* cpu_opcode_table[256] =
{
	"BRK","ORA","*KIL","*SLO","*NOP","ORA","ASL","*SLO","PHP","ORA","ASL","*ANC","*NOP","ORA","ASL","*SLO",
	"BPL","ORA","*KIL","*SLO","*NOP","ORA","ASL","*SLO","CLC","ORA","*NOP","*SLO","*NOP","ORA","ASL","*SLO",
	"JSR","AND","*KIL","*RLA","BIT","AND","ROL","*RLA","PLP","AND","ROL","*ANC","BIT","AND","ROL","*RLA",
	"BMI","AND","*KIL","*RLA","*NOP","AND","ROL","*RLA","SEC","AND","*NOP","*RLA","*NOP","AND","ROL","*RLA",
	"RTI","EOR","*KIL","*SRE","*NOP","EOR","LSR","*SRE","PHA","EOR","LSR","*ALR","JMP","EOR","LSR","*SRE",
	"BVC","EOR","*KIL","*SRE","*NOP","EOR","LSR","*SRE","CLI","EOR","*NOP","*SRE","*NOP","EOR","LSR","*SRE",
	"RTS","ADC","*KIL","*RRA","*NOP","ADC","ROR","*RRA","PLA","ADC","ROR","*ARR","JMP","ADC","ROR","*RRA",
	"BVS","ADC","*KIL","*RRA","*NOP","ADC","ROR","*RRA","SEI","ADC","*NOP","*RRA","*NOP","ADC","ROR","*RRA",
	"*NOP","STA","*NOP","*SAX","STY","STA","STX","*SAX","DEY","*NOP","TXA","*XAA","STY","STA","STX","*SAX",
	"BCC","STA","*KIL","*AHX","STY","STA","STX","*SAX","TYA","STA","TXS","*TAS","*SHY","STA","*SHX","*AHX",
	"LDY","LDA","LDX","*LAX","LDY","LDA","LDX","*LAX","TAY","LDA","TAX","*LAX","LDY","LDA","LDX","*LAX",
	"BCS","LDA","*KIL","*LAX","LDY","LDA","LDX","*LAX","CLV","LDA","TSX","*LAS","LDY","LDA","LDX","*LAX",
	"CPY","CMP","*NOP","*DCP","CPY","CMP","DEC","*DCP","INY","CMP","DEX","*AXS","CPY","CMP","DEC","*DCP",
	"BNE","CMP","*KIL","*DCP","*NOP","CMP","DEC","*DCP","CLD","CMP","*NOP","*DCP","*NOP","CMP","DEC","*DCP",
	"CPX","SBC","*NOP","*ISC","CPX","SBC","INC","*ISC","INX","SBC","NOP","*SBC","CPX","SBC","INC","*ISC",
	"BEQ","SBC","*KIL","*ISC","*NOP","SBC","INC","*ISC","SED","SBC","*NOP","*ISC","*NOP","SBC","INC","*ISC"
};

#define _IMP	0
#define _ACC	1
#define _REL	2
#define _ZPG	3
#define _ZPX	4
#define _ZPY	5
#define _ABS	6
#define _ABX	7
#define _ABY	8
#define _IND	9
#define _IZX	10
#define _IZY	11
#define _IMM	12

static const unsigned char cpu_addr_table[256] =
{
_IMP,_IZX,_IMP,_IZX,_ZPG,_ZPG,_ZPG,_ZPG,_IMP,_IMM,_ACC,_IMM,_ABS,_ABS,_ABS,_ABS,
_REL,_IZY,_IMP,_IZY,_ZPX,_ZPX,_ZPX,_ZPX,_IMP,_ABY,_IMP,_ABY,_ABX,_ABX,_ABX,_ABX,
_ABS,_IZX,_IMP,_IZX,_ZPG,_ZPG,_ZPG,_ZPG,_IMP,_IMM,_ACC,_IMM,_ABS,_ABS,_ABS,_ABS,
_REL,_IZY,_IMP,_IZY,_ZPX,_ZPX,_ZPX,_ZPX,_IMP,_ABY,_IMP,_ABY,_ABX,_ABX,_ABX,_ABX,
_IMP,_IZX,_IMP,_IZX,_ZPG,_ZPG,_ZPG,_ZPG,_IMP,_IMM,_ACC,_IMM,_ABS,_ABS,_ABS,_ABS,
_REL,_IZY,_IMP,_IZY,_ZPX,_ZPX,_ZPX,_ZPX,_IMP,_ABY,_IMP,_ABY,_ABX,_ABX,_ABX,_ABX,
_IMP,_IZX,_IMP,_IZX,_ZPG,_ZPG,_ZPG,_ZPG,_IMP,_IMM,_ACC,_IMM,_IND,_ABS,_ABS,_ABS,
_REL,_IZY,_IMP,_IZY,_ZPX,_ZPX,_ZPX,_ZPX,_IMP,_ABY,_IMP,_ABY,_ABX,_ABX,_ABX,_ABX,
_IMM,_IZX,_IMM,_IZX,_ZPG,_ZPG,_ZPG,_ZPG,_IMP,_IMM,_IMP,_IMM,_ABS,_ABS,_ABS,_ABS,
_REL,_IZY,_IMP,_IZY,_ZPX,_ZPX,_ZPY,_ZPY,_IMP,_ABY,_IMP,_ABY,_ABX,_ABX,_ABY,_ABY,
_IMM,_IZX,_IMM,_IZX,_ZPG,_ZPG,_ZPG,_ZPG,_IMP,_IMM,_IMP,_IMM,_ABS,_ABS,_ABS,_ABS,
_REL,_IZY,_IMP,_IZY,_ZPX,_ZPX,_ZPY,_ZPY,_IMP,_ABY,_IMP,_ABY,_ABX,_ABX,_ABY,_ABY,
_IMM,_IZX,_IMM,_IZX,_ZPG,_ZPG,_ZPG,_ZPG,_IMP,_IMM,_IMP,_IMM,_ABS,_ABS,_ABS,_ABS,
_REL,_IZY,_IMP,_IZY,_ZPX,_ZPX,_ZPX,_ZPX,_IMP,_ABY,_IMP,_ABY,_ABX,_ABX,_ABX,_ABX,
_IMM,_IZX,_IMM,_IZX,_ZPG,_ZPG,_ZPG,_ZPG,_IMP,_IMM,_IMP,_IMM,_ABS,_ABS,_ABS,_ABS,
_REL,_IZY,_IMP,_IZY,_ZPX,_ZPX,_ZPX,_ZPX,_IMP,_ABY,_IMP,_ABY,_ABX,_ABX,_ABX,_ABX,
};

int Dasm(unsigned short _pc, char *dasm_buffer)
{
	int size = 0;
	pair_t _addr;
	unsigned char opcode = bus::Read(_pc);
	unsigned char addr8 = bus::Read(_pc + 1);
	signed char rel = (signed char) addr8;
	_addr.LO = addr8;
	_addr.HI = bus::Read(_pc + 2);

    size = 1;
	switch (cpu_addr_table[opcode])
	{
	case _IMP:
		sprintf(dasm_buffer, "%s", cpu_opcode_table[opcode]);
		break;
	case _ACC:
		sprintf(dasm_buffer, "%s A", cpu_opcode_table[opcode]);
		break;
	case _REL:
        size = 2;
        _addr.W = (unsigned)((signed short) (_pc+2) + rel);
		sprintf(dasm_buffer, "%s $%04X", cpu_opcode_table[opcode], _addr.W);
		break;
	case _ZPG:
        size = 2;
		sprintf(dasm_buffer, "%s $%02X", cpu_opcode_table[opcode], addr8);
		break;
	case _ZPX:
        size = 2;
		sprintf(dasm_buffer, "%s $%02X, X", cpu_opcode_table[opcode], addr8);
		break;
	case _ZPY:
        size = 2;
		sprintf(dasm_buffer, "%s $%02X, Y", cpu_opcode_table[opcode], addr8);
		break;
	case _ABS:
        size = 3;
		sprintf(dasm_buffer, "%s $%04X", cpu_opcode_table[opcode], _addr.W);
		break;
	case _ABX:
		size = 3;
		sprintf(dasm_buffer, "%s $%04X, X", cpu_opcode_table[opcode], _addr.W);
		break;
	case _ABY:
        size = 3;
		sprintf(dasm_buffer, "%s $%04X, Y", cpu_opcode_table[opcode], _addr.W);
		break;
	case _IND:
        size = 3;
		sprintf(dasm_buffer, "%s ($%04X)", cpu_opcode_table[opcode], _addr.W);
		break;
	case _IZX:
        size = 2;
		sprintf(dasm_buffer, "%s ($%02X, X)", cpu_opcode_table[opcode], addr8);
		break;
	case _IZY:
        size = 2;
		sprintf(dasm_buffer, "%s ($%02X), Y", cpu_opcode_table[opcode], addr8);
		break;
	case _IMM:
        size = 2;
		sprintf(dasm_buffer, "%s #$%02X", cpu_opcode_table[opcode], addr8);
		break;
	}
	return size;
}

void Debug()
{
	int value=0;
	printf("1 - Step Into\n");
	printf("2 - Show Regs\n");
	printf("3 - Dump stack\n");
	printf("4 - Dump memory\n");
	printf("9 - Exit\n");
	unsigned dumpstart = 0;
	do
	{
		printf(">");
		scanf("%d", &value);

		switch (value)
		{
		case 1:
			//printf("%s\n", Dasm(PC.W));
            TraceToConsole();
			Run(1);
			break;
		case 2:
			printf("A:%02X X:%02X Y:%02X S:%02X PC: %04X\n", A, X, Y, S, PC.W);
			break;
		case 3:
			dumpstart = 0x100;
			goto _DUMPMEMORY;
		case 4:
			printf(">>"); scanf("%X", &dumpstart);
			goto _DUMPMEMORY;
_DUMPMEMORY:
			{
				for (int line = 0; line < 0x100; line+=0x10)
				{
					printf("$%04X | ", line+dumpstart);
					for (int column = 0; column < 0x10; column++)
						printf("%02X ", bus::Read(dumpstart+line+column));
					printf("| ");
					{
						char chr = ' ';
						for (int column = 0; column < 0x10; column++)
						{
							chr = bus::Read(dumpstart+line+column);
							if (isalpha(chr) || isdigit(chr))
                                printf("%c", chr);
                            else
                                printf(".");
						}
					}
					printf("\n");
				}
			}
			break;
		}
	} while (value != 9);
}

};

