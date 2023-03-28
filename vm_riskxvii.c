#include <stdio.h>

#define OP_TYPE_R  0b0110011
#define OP_TYPE_I  0b0010011
#define OP_TYPE_U  0b0110111
#define OP_TYPE_S  0b0100011
#define OP_TYPE_SB 0b1100011
#define OP_TYPE_UJ 0b1101111
#define OP_MASK 0x7F
#define FUNC3_MASK 0x7
#define RD_MASK 0x1F
#define RS1_MASK 0x1F
#define RS2_MASK 0x1F
#define FUNC7_MASK 0x7F

#define TYPE_I_IMM_MASK 0xFFF
#define TYPE_U_IMM_MASK 0xFFFFF000
#define TYPE_S_IMM_1_MASK 0x7F
#define TYPE_S_IMM_2_MASK 0x1F
#define TYPE_UJ_IMM_MASK 0xFFFFF

unsigned int GET_OP(unsigned int val)
{
	return (val & OP_MASK);
}

unsigned int GET_RD(unsigned int val)
{
	return ((val >> 7) & RD_MASK);
}

unsigned int GET_FUNC3(unsigned int val)
{
	return ((val >> 12) & FUNC3_MASK);
}

unsigned int GET_RS1(unsigned int val)
{
	return ((val >> 15) & RS1_MASK);
}

unsigned int GET_RS2(unsigned int val)
{
	return ((val >> 20) & RS2_MASK);
}

unsigned int GET_FUNC7(unsigned int val)
{
	return ((val >> 25) & FUNC7_MASK);
}

unsigned int GET_TYPE_I_IMM(unsigned int val)
{
	return ((val >> 20) & TYPE_I_IMM_MASK);
}

unsigned int GET_TYPE_S_IMM_1(unsigned int val)
{
	return ((val >> 25) & TYPE_S_IMM_1_MASK);
}

unsigned int GET_TYPE_S_IMM_2(unsigned int val)
{
	return ((val >> 7) & TYPE_S_IMM_2_MASK);
}


int analyze_seq(unsigned int val, unsigned int reg[], unsigned int Mem[], unsigned int *pc)
{
	unsigned int op = GET_OP(val);
	unsigned int func7, func3, rs1, rs2, rd, imm1, imm2, imm;

	switch(op)
	{
		case OP_TYPE_R:
			func7 = GET_FUNC7(val);
			rs2 = GET_RS2(val);
			rs1 = GET_RS1(val);
			rd = GET_RD(val);
			func3 = GET_FUNC3(val);

			if(func3 == 0b000 && func7 == 0) { //add
				reg[rd] = reg[rs1] + reg[rs2];
			} else if (func3 == 0b000 && func7 == 0b0100000) { //sub
				reg[rd] = reg[rs1] - reg[rs2];
			} else if (func3 == 0b100 && func7 == 0) { //xor
				reg[rd] = reg[rs1] ^ reg[rs2];
			} else if (func3 == 0b110 && func7 == 0) { //or
				reg[rd] = reg[rs1] | reg[rs2];
			} else if (func3 == 0b111 && func7 == 0) { //and
				reg[rd] = reg[rs1] & reg[rs2];
			} else if (func3 == 0b001 && func7 == 0) { //sll
				reg[rd] = reg[rs1] << reg[rs2];
			} else if (func3 == 0b101 && func7 == 0) { //srl
				reg[rd] = reg[rs1] >> reg[rs2];
			} else if (func3 == 0b101 && func7 == 0b0100000) { //sra
				unsigned int mask = (2 << reg[rs2]) - 1;
				reg[rd] = reg[rs1] >> reg[rs2] | ((reg[rs1] & mask ) << (32 - reg[rs2]));
			//program flow
			} else if (func3 == 0b010 && func7 == 0) { //slt
				reg[rd] = (reg[rs1] < reg[rs2]) ? 1 : 0;
			//??? how to express signed or unsigned ???
			} else if (func3 == 0b011 && func7 == 0) { //sltu
				reg[rd] = (reg[rs1] < reg[rs2]) ? 1 : 0;
			}
			break;
		case OP_TYPE_I:
			func7 = GET_FUNC7(val);
			rs2 = GET_RS2(val);
			rs1 = GET_RS1(val);
			rd = GET_RD(val);
			func3 = GET_FUNC3(val);
			imm1 = GET_TYPE_I_IMM(val);
		
			if(func3 == 0) { //addi
				reg[rd] = reg[rs1] + imm1;
			} else if(func3 == 0b100 && func7 == 0b0010011) { //xori
				reg[rd] = reg[rs1] ^ imm1;
			} else if(func3 == 0b110) { //ori
				reg[rd] = reg[rs1] | imm1;
			} else if(func3 == 0b111) { //andi
				reg[rd] = reg[rs1] & imm1;
			} else if(func3 == 0) { //lb Todo //consider mem as 4 byte unit
				reg[rd] = (int)(Mem[reg[rs1] + imm1] & 0xFF);
			} else if(func3 == 1) { //lh Todo
				reg[rd] = (int)(Mem[reg[rs1] + imm1] & 0xFFFF);
			} else if(func3 == 0b010) { //lw Todo
				reg[rd] = (Mem[reg[rs1] + imm1]);
			} else if(func3 == 0b100) { //lbu Todo
				reg[rd] = Mem[reg[rs1] + imm1] & 0xFF;
			} else if(func3 == 0b101) { //lhu Todo
				reg[rd] = Mem[reg[rs1] + imm1] & 0xFFFF;
			//program flow
			} else if (func3 == 0b010) { //slti
				reg[rd] = (reg[rs1] < imm1) ? 1 : 0;
			//??? how to express signed or unsigned ???
			} else if (func3 == 0b011) { //sltiu 
				reg[rd] = (reg[rs1] < imm1) ? 1 : 0;
			} else if (func3 == 0b011) { //jalr
				reg[rd]= *pc + 4;
				*pc = reg[rs1] + imm1;
			}
			break;
		case OP_TYPE_S:
			imm1 = GET_TYPE_S_IMM_1(val);
			imm2 = GET_TYPE_S_IMM_2(val);
			imm = imm1 << 5 | imm2;
			rs2 = GET_RS2(val);
			rs1 = GET_RS1(val);
			func3 = GET_FUNC3(val);

			if(func3 == 0b0) { //sb
				Mem[reg[rs1] + imm] = reg[rs2] & 0xFF;
			} else if(func3 == 0b001) { //sh
				Mem[reg[rs1] + imm] = reg[rs2] & 0xFFFF;
			} else if(func3 == 0b010) { //sw
				Mem[reg[rs1] + imm] = reg[rs2];
			}
			break;
		case OP_TYPE_U:
			rd = GET_RD(val);
			reg[rd] = (val & TYPE_U_IMM_MASK) << 12;

			break;
		case OP_TYPE_UJ: //jal
			rd = GET_RD(val);
			imm1 = val & TYPE_U_IMM_MASK;
			imm = ((imm1 >> 9) & 0x3FF) | (((imm1 >> 8) & 0x1) << 10) | (((imm1) & 0xFF) << 11) | ((imm1 >> 19) << 19);
			reg[rd] = *pc + 4;
			*pc = *pc + (imm << 1);

			break;
		case OP_TYPE_SB:
			rs2 = GET_RS2(val);
			rs1 = GET_RS1(val);
			func3 = GET_FUNC3(val);
			imm1 = GET_TYPE_S_IMM_1(val);
			imm2 = GET_TYPE_S_IMM_2(val);
			imm = (imm2 >> 1) | ((imm1 & 0x3F) << 4) | ((imm2 & 0x1) << 10) | ((imm1 >> 6) << 11);

			if(func3 == 0b0) { //beq
				if(reg[rs1] == reg[rs2]) {
					*pc = *pc + (imm << 1);
				}
			} else if(func3 == 0b001) { //bneq 
				if(reg[rs1] != reg[rs2]) {
					*pc = *pc + (imm << 1);
				}
			} else if(func3 == 0b100) { //blt 
				if(reg[rs1] < reg[rs2]) {
					*pc = *pc + (imm << 1);
				}
			//??? how to express signed or unsigned ???
			} else if(func3 == 0b110) { //bltu
				if(reg[rs1] < reg[rs2]) {
					*pc = *pc + (imm << 1);
				}
			} else if(func3 == 0b101) { //bge 
				if(reg[rs1] >= reg[rs2]) {
					*pc = *pc + (imm << 1);
				}
			} else if(func3 == 0b111) { //bgeu
				if(reg[rs1] >= reg[rs2]) {
					*pc = *pc + (imm << 1);
				}
			}
			break;
		default:
			break;
	}
	return 0;
}

int main(){
	unsigned int val = 0x00f686b3;
	unsigned int reg[32] =  { 0 };
	unsigned int Mem[256] = { 0 };
	unsigned int pc;
	analyze_seq(val, reg, Mem, &pc);
	
    return 1;
}
