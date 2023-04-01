#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OP_TYPE_R  0b0110011
#define OP_TYPE_I  0b0010011
#define OP_TYPE_U  0b0110111
#define OP_TYPE_S  0b0100011
#define OP_TYPE_SB 0b1100011
#define OP_TYPE_UJ 0b1101111
#define OP_TYPE_I2 0b1100111
#define OP_TYPE_I3 0b0000011
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

#define CONSOLE_WRITE_CHAR 0x800
#define CONSOLE_WRITE_SIGNED_INT 0x804
#define CONSOLE_WRITE_UNSIGNED_INT 0x808
#define HALT 0x80C
#define CONSOLE_READ_CHAR 0x812
#define CONSOLE_READ_SIGNED_INT 0x816
#define DUMP_PC 0x820
#define DUMP_REG_BANK 0x824
#define DUMP_MEM_WORD 0x828
#define MALLOC_MEM 0x830
#define FREE_MEM 0x834

#define INST_MEM_SIZE 1024
#define DATA_MEM_SIZE 1024

typedef struct {
	char inst_mem[INST_MEM_SIZE];
	char data_mem[DATA_MEM_SIZE];
}blob;

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


int analyze_seq(unsigned int *inst, unsigned int reg[], unsigned char Mem[], unsigned int *pc)
{
	unsigned int val = inst[*pc/4];
	unsigned int op = GET_OP(val);
	unsigned int func7, func3, rs1, rs2, rd, imm1, imm2, imm;
	unsigned int temp;
	/* int nextCmd = 0; */


	/* printf("0x%x\n", val); */

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
			if((imm1 >> 11) == 1) {
				imm1 = 0xFFFFF000|imm1;
			}
		
			if(func3 == 0) { //addi
				reg[rd] = reg[rs1] + imm1;
			} else if(func3 == 0b100 && func7 == 0b0010011) { //xori
				reg[rd] = reg[rs1] ^ imm1;
			} else if(func3 == 0b110) { //ori
				reg[rd] = reg[rs1] | imm1;
			} else if(func3 == 0b111) { //andi
				reg[rd] = reg[rs1] & imm1;
			//program flow
			} else if (func3 == 0b010) { //slti
				reg[rd] = (reg[rs1] < imm1) ? 1 : 0;
			//??? how to express signed or unsigned ???
			} else if (func3 == 0b011) { //sltiu 
				reg[rd] = (reg[rs1] < imm1) ? 1 : 0;
			}
			break;
		case OP_TYPE_I2:
			func7 = GET_FUNC7(val);
			rs2 = GET_RS2(val);
			rs1 = GET_RS1(val);
			rd = GET_RD(val);
			func3 = GET_FUNC3(val);
			imm1 = GET_TYPE_I_IMM(val);
			if((imm1 >> 11) == 1) {
				imm1 = 0xFFFFF000|imm1;
			}
			if (func3 == 0) { //jalr
				reg[rd]= *pc + 4;
				*pc = reg[rs1] + imm1;
				return 0;
			}
		case OP_TYPE_I3:
			func7 = GET_FUNC7(val);
			rs2 = GET_RS2(val);
			rs1 = GET_RS1(val);
			rd = GET_RD(val);
			func3 = GET_FUNC3(val);
			imm1 = GET_TYPE_I_IMM(val);
			if((imm1 >> 11) == 1) {
				imm1 = 0xFFFFF000|imm1;
			}
			//load 
			temp = reg[rs1] + imm1;
			if(temp == CONSOLE_READ_CHAR){
				reg[rd] = getchar();
			} else if (temp == CONSOLE_READ_SIGNED_INT) {
				scanf("%d",&reg[rd]);
			} else if(func3 == 0) { //lb Todo //consider mem as 4 byte unit
				reg[rd] = (int)Mem[reg[rs1] + imm1] ;
			} else if(func3 == 1) { //lh Todo
				reg[rd] = (int)(Mem[reg[rs1] + imm1] +  (Mem[reg[rs1] + imm1 + 1] << 8));
			} else if(func3 == 0b010) { //lw Todo
				reg[rd] = (int)(Mem[reg[rs1] + imm1] +  (Mem[reg[rs1] + imm1 + 1] << 8));
				reg[rd] += (int)((Mem[reg[rs1] + imm1 + 2] << 16) +  (Mem[reg[rs1] + imm1 + 3] << 24));
			} else if(func3 == 0b100) { //lbu Todo
				reg[rd] = Mem[reg[rs1] + imm1] & 0xFF;
			} else if(func3 == 0b101) { //lhu Todo
				reg[rd] = Mem[reg[rs1] + imm1] & 0xFFFF;
			}
			break;
		case OP_TYPE_S:
			imm1 = GET_TYPE_S_IMM_1(val);
			imm2 = GET_TYPE_S_IMM_2(val);
			imm = imm1 << 5 | imm2;
			rs2 = GET_RS2(val);
			rs1 = GET_RS1(val);
			func3 = GET_FUNC3(val);
			if((imm >> 11) == 1) {
				imm = 0xFFFFF000|imm;
			}

			temp = reg[rs1] + imm;

			//virtual routines
			if(temp == CONSOLE_WRITE_CHAR) {
				printf("%c",reg[rs2]);
			} else if(temp == CONSOLE_WRITE_SIGNED_INT) {
				printf("%d",reg[rs2]);
			} else if(temp == CONSOLE_WRITE_UNSIGNED_INT) {
				printf("%u",reg[rs2]);
			} else if(temp == HALT) {
				printf("CPU Halt Requested\n");
				exit(1);
			} else if(func3 == 0b0) { //sb
				Mem[reg[rs1] + imm] = reg[rs2] & 0xFF;
			} else if(func3 == 0b001) { //sh
				Mem[reg[rs1] + imm] = reg[rs2] & 0xFFFF;
			} else if(func3 == 0b010) { //sw
				Mem[reg[rs1] + imm] = reg[rs2];
			}
			
			break;
		case OP_TYPE_U: //lui
			rd = GET_RD(val);
			//imm bit0 -> bit11 is 0
			reg[rd] = (val & TYPE_U_IMM_MASK);

			break;
		case OP_TYPE_UJ: //jal
			rd = GET_RD(val);
			imm1 = (val & TYPE_U_IMM_MASK) >> 12;
			imm = ((imm1 >> 9) & 0x3FF) | (((imm1 >> 8) & 0x1) << 10) | (((imm1) & 0xFF) << 11) | ((imm1 >> 19) << 19);
			if((imm >> 19) == 1) {
				imm = 0xFFF80000|imm;
			}
			reg[rd] = *pc + 4;
			*pc = *pc + (imm << 1);

			return 0;
		case OP_TYPE_SB:
			rs2 = GET_RS2(val);
			rs1 = GET_RS1(val);
			func3 = GET_FUNC3(val);
			imm1 = GET_TYPE_S_IMM_1(val);
			imm2 = GET_TYPE_S_IMM_2(val);
			imm = (imm2 >> 1) | ((imm1 & 0x3F) << 4) | ((imm2 & 0x1) << 10) | ((imm1 >> 6) << 11);
			if((imm >> 11) == 1) {
				imm = 0xFFFFF000|imm;
			}

			if(func3 == 0b0) { //beq
				if(reg[rs1] == reg[rs2]) {
					*pc = *pc + (imm << 1);
					return 0;
				}
			} else if(func3 == 0b001) { //bneq 
				if(reg[rs1] != reg[rs2]) {
					*pc = *pc + (imm << 1);
					return 0;
				}
			} else if(func3 == 0b100) { //blt 
				if(reg[rs1] < reg[rs2]) {
					*pc = *pc + (imm << 1);
					return 0;
				}
			//??? how to express signed or unsigned ???
			} else if(func3 == 0b110) { //bltu
				if(reg[rs1] < reg[rs2]) {
					*pc = *pc + (imm << 1);
					return 0;
				}
			} else if(func3 == 0b101) { //bge 
				if(reg[rs1] >= reg[rs2]) {
					*pc = *pc + (imm << 1);
					return 0;
				}
			} else if(func3 == 0b111) { //bgeu
				if(reg[rs1] >= reg[rs2]) {
					*pc = *pc + (imm << 1);
					return 0;
				}
			}
			break;
		default:
			return 1;
	}
	*pc += 4;
	return 0;
}

/* #define uint32_t (unsigned int) */
/* #define BSWAP_32(x)  ((uint32_t)((((uint32_t)(x) & 0xff000000) >> 24) | \ */
                    /* (((uint32_t)(x) & 0x00ff0000) >> 8) | \ */
                    /* (((uint32_t)(x) & 0x0000ff00) << 8) | \ */
                    /* (((uint32_t)(x) & 0x000000ff) << 24) ) ) */

/* #define BSWAP_32(x)  ((((x) & 0xff000000) >> 24) | \ */
                    /* (((x) & 0x00ff0000) >> 8) | \ */
                    /* (((x) & 0x0000ff00) << 8) | \ */
                    /* (((x) & 0x000000ff) << 24) )  */

int main(int argc, char *argv[]){
	unsigned int reg[32] =  { 0 };
	unsigned char Mem[1024] = { 0 };
	unsigned int pc;
	FILE* in_file;
	blob cmds;
	unsigned int *ptr;
	int ret = 0;

	if(argc == 2) {
		in_file = fopen(argv[1], "rb");
		fread(&cmds, INST_MEM_SIZE + DATA_MEM_SIZE, 1, in_file);
		/* printf("0x%x, 0x%x 0x%x 0x%x\n", cmds.inst_mem[0], cmds.inst_mem[1], cmds.inst_mem[2], cmds.inst_mem[3]); */
		ptr =(unsigned int*)&cmds.inst_mem;
#if 0
		for(int i=0; i< 2048/4; i++) {
			if(i%4 == 0)
				printf("\n");
			printf("0x%x ", *ptr);
			/* printf("0x%x\n", BSWAP_32(*ptr)); */
			/* analyze_seq(*ptr, reg, Mem, ptr); */
			ptr++;
		}
#endif
	}

	ptr = (unsigned int*)&cmds.inst_mem;
	while(1) {
		ret = analyze_seq(ptr, reg, Mem, &pc);
		if(ret == 1)
			break;
	}

#if 0 
	unsigned int test[10] = { 0 };
	test[0] = 0x7ff00113;
	test[1] = 0x00c000ef;
	test[2] = 0x000017b7;
	test[3] = 0x80078623;
	test[4] = 0x000017b7;
	test[5] = 0x04800713;
	test[6] = 0x80e78023;
	test[7] = 0x00000513;
	test[8] = 0x00008067;
	for(int i = 4; i < 7; i++) {
	/* for(int i = 0; i < 9; i++) { */
		analyze_seq(test[i], reg, Mem, &pc);
	}
#endif
    return 1;
}
