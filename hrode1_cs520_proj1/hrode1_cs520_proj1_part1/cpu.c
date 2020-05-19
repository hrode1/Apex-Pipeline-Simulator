/*
 *  cpu.c
 *  Contains APEX cpu pipeline implementation
 *
 *  Author :
 *  Gaurav Kothari (gkothar1@binghamton.edu)
 *  State University of New York, Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

/* Set this flag to 1 to enable debug messages */
#define ENABLE_DEBUG_MESSAGES 1

int mulCycleCounter=0;
int mulEXtoMEM=0;
int stopSimulation=0;
int display=0;
int zeroFlag=0;
int justFetchinDRF=0;
int alreadyFetched=0;
int branchToEX=0;
struct CPU_Stage nop = {0, "NOP", 0, 0, 0, 0};

/*
 * This function creates and initializes APEX cpu.
 *
 * Note : You are free to edit this function according to your
 * 				implementation
 */
APEX_CPU*
APEX_cpu_init(const char* filename)
{
  if (!filename) {
    return NULL;
  }

  APEX_CPU* cpu = malloc(sizeof(*cpu));
  if (!cpu) {
    return NULL;
  }

  /* Initialize PC, Registers and all pipeline stages */
  cpu->pc = 4000;
  memset(cpu->regs, 0, sizeof(int) * 32);
  memset(cpu->regs_valid, 1, sizeof(int) * 32);
  memset(cpu->stage, 0, sizeof(CPU_Stage) * NUM_STAGES);
  memset(cpu->data_memory, 0, sizeof(int) * 4000);

  /* Parse input file and create code memory */
  cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);

  if (!cpu->code_memory) {
    free(cpu);
    return NULL;
  }

  if (ENABLE_DEBUG_MESSAGES) {
    fprintf(stderr,
            "APEX_CPU : Initialized APEX CPU, loaded %d instructions\n",
            cpu->code_memory_size);
    fprintf(stderr, "APEX_CPU : Printing Code Memory\n");
    printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode", "rd", "rs1", "rs2", "imm");

    for (int i = 0; i < cpu->code_memory_size; ++i) {
      printf("%-9s %-9d %-9d %-9d %-9d\n",
             cpu->code_memory[i].opcode,
             cpu->code_memory[i].rd,
             cpu->code_memory[i].rs1,
             cpu->code_memory[i].rs2,
             cpu->code_memory[i].imm);
    }
  }

  /* Make all stages busy except Fetch stage, initally to start the pipeline */
  for (int i = 1; i < NUM_STAGES; ++i) {
    cpu->stage[i].busy = 1;
  }

  return cpu;
}

/*
 * This function de-allocates APEX cpu.
 *
 * Note : You are free to edit this function according to your
 * 				implementation
 */
void
APEX_cpu_stop(APEX_CPU* cpu)
{
  free(cpu->code_memory);
  free(cpu);
}

/* Converts the PC(4000 series) into
 * array index for code memory
 *
 * Note : You are not supposed to edit this function
 *
 */
int
get_code_index(int pc)
{
  return (pc - 4000) / 4;
}

static void
print_instruction(CPU_Stage* stage)
{
  if (strcmp(stage->opcode, "STORE") == 0) {
    printf( "%s,R%d,R%d,#%d ", stage->opcode, stage->rs1, stage->rs2, stage->imm);
  }
  
  if (strcmp(stage->opcode, "LOAD") == 0) {
    printf( "%s,R%d,R%d,#%d ", stage->opcode, stage->rd, stage->rs1, stage->imm);
  }

  if (strcmp(stage->opcode, "MOVC") == 0) {
    printf("%s,R%d,#%d ", stage->opcode, stage->rd, stage->imm);
  }

  if (strcmp(stage->opcode, "ADD") == 0) {
    printf(
      "%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }

  if (strcmp(stage->opcode, "SUB") == 0) {
    printf(
      "%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }

  if (strcmp(stage->opcode, "AND") == 0) {
    printf(
      "%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }

  if (strcmp(stage->opcode, "OR") == 0) {
    printf(
      "%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }

  if (strcmp(stage->opcode, "EX-OR") == 0) {
    printf(
      "%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }
  
  if (strcmp(stage->opcode, "MUL") == 0) {
    printf(
      "%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }
  
  if (strcmp(stage->opcode, "HALT") == 0) {
    printf("%s ", stage->opcode);
  }
  
  if (strcmp(stage->opcode, "BZ") == 0) {
    printf(
      "%s,#%d ", stage->opcode, stage->imm);
  } 
  
  if (strcmp(stage->opcode, "BNZ") == 0) {
    printf(
      "%s,#%d ", stage->opcode, stage->imm);
  }
  
  if (strcmp(stage->opcode, "JUMP") == 0) {
    printf(
      "%s,R%d,#%d ", stage->opcode, stage->rs1, stage->imm);
  }  
}

/* Debug function which dumps the cpu stage
 * content
 *
 * Note : You are not supposed to edit this function
 *
 */
static void
print_stage_content(char* name, CPU_Stage* stage)
{
	if (display == 1) {
		printf("%-15s: pc(%d) ", name, stage->pc);
		print_instruction(stage);
		printf("\n");
	}
}

/*
 *  Fetch Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
fetch(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[F];
  //printf("cpu->stage[F].stalled : %d\n",cpu->stage[F].stalled);	
  if (!stage->busy && !stage->stalled) {  
    if(alreadyFetched==1)
	{
	  alreadyFetched=0;
	  cpu->stage[DRF] = cpu->stage[F];
	  print_stage_content("Fetch", stage);
	  return 0;
	}
    /* Store current PC in fetch latch */
    stage->pc = cpu->pc;

    /* Index into code memory using this pc and copy all instruction fields into
     * fetch latch
     */
    APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
    strcpy(stage->opcode, current_ins->opcode);
    stage->rd = current_ins->rd;
    stage->rs1 = current_ins->rs1;
    stage->rs2 = current_ins->rs2;
    stage->imm = current_ins->imm;
    stage->rd = current_ins->rd;

    /* Update PC for next instruction */
    cpu->pc += 4;

    /* Copy data from fetch latch to decode latch*/
    cpu->stage[DRF] = cpu->stage[F];

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Fetch", stage);
    }
  }
  else
  {
	  if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Fetch", stage);
    }
  }
  return 0;
}

/*
 *  Decode Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
decode(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[DRF];
  if (!stage->busy && !stage->stalled) {
	  //printf("cpu->stage[F].stalled Decode: %d\n",cpu->stage[F].stalled);
	  //printf("stage->opcode : %s\n",stage->opcode);
	if ( ((strcmp(stage->opcode, "STORE") == 0 || strcmp(stage->opcode, "ADD") == 0 || strcmp(stage->opcode, "SUB") == 0 ||
		strcmp(stage->opcode, "AND") == 0 || strcmp(stage->opcode, "OR") == 0 || strcmp(stage->opcode, "EX-OR") == 0 ||
		strcmp(stage->opcode, "MUL") == 0) && (cpu->regs_valid[stage->rs1] != 1) && (cpu->regs_valid[stage->rs2] != 1))
		|| (strcmp(stage->opcode, "BZ") == 0)
		|| (strcmp(stage->opcode, "NOP") == 0)
		|| (strcmp(stage->opcode, "BNZ") == 0)
		|| (strcmp(stage->opcode, "MOVC") == 0)
		|| (strcmp(stage->opcode, "HALT") == 0)
		|| ((strcmp(stage->opcode, "LOAD") == 0 || strcmp(stage->opcode, "JUMP") == 0) && (cpu->regs_valid[stage->rs1] != 1)) )
		{
			//printf("In decode...\n");
			if (strcmp(stage->opcode, "BZ") == 0 || strcmp(stage->opcode, "BNZ") == 0) {
				branchToEX++;
				if ( (strcmp(cpu->stage[EX].opcode,"ADD") == 0) || 
					 (strcmp(cpu->stage[EX].opcode,"SUB") == 0) || 
					 (strcmp(cpu->stage[EX].opcode,"MUL") == 0) )
				{
					print_stage_content("Decode/RF", stage);
					cpu->stage[EX]=nop;
					
					/* Only fetching the instruction and not incrementing stage pointer */
					cpu->stage[F].pc = cpu->pc;
					APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
					strcpy(cpu->stage[F].opcode, current_ins->opcode);
					cpu->stage[F].rd = current_ins->rd;
					cpu->stage[F].rs1 = current_ins->rs1;
					cpu->stage[F].rs2 = current_ins->rs2;
					cpu->stage[F].imm = current_ins->imm;
					cpu->stage[F].rd = current_ins->rd;
					cpu->pc += 4;
					
					cpu->stage[F].stalled=1;
					
					return 0;
				}
				if(branchToEX==3) {
					branchToEX=0;
					cpu->stage[EX] = cpu->stage[DRF];
					cpu->stage[F].stalled=0;
					alreadyFetched=1;
					print_stage_content("Decode/RF", stage);
					return 0;
				}
				print_stage_content("Decode/RF", stage);
				return 0;
			}
			
			/* Read data from register file */
			if (strcmp(stage->opcode, "STORE") == 0 || strcmp(stage->opcode, "ADD") == 0 || strcmp(stage->opcode, "SUB") == 0 ||
				strcmp(stage->opcode, "AND") == 0 || strcmp(stage->opcode, "OR") == 0 || strcmp(stage->opcode, "EX-OR") == 0 ||
				strcmp(stage->opcode, "MUL") == 0) {
				stage->rs1_value=cpu->regs[stage->rs1];
				stage->rs2_value=cpu->regs[stage->rs2];
			}
			
			/* Read data from register file for LOAD */
			if (strcmp(stage->opcode, "LOAD") == 0 || strcmp(stage->opcode, "JUMP") == 0) {
				stage->rs1_value=cpu->regs[stage->rs1];
			}

			if (ENABLE_DEBUG_MESSAGES) {
			  print_stage_content("Decode/RF", stage);
			}
			/* Copy data from decode latch to execute latch*/
			cpu->stage[EX] = cpu->stage[DRF];
			justFetchinDRF=0;
			cpu->stage[F].stalled =0;
		}
		else
		{
			cpu->stage[EX] = nop;
			print_stage_content("Decode/RF", stage);
			justFetchinDRF++;
			if(justFetchinDRF==1) {
				/* Only fetching the instruction and not incrementing stage pointer */
				/* Store current PC in fetch latch */
				cpu->stage[F].pc = cpu->pc;

				/* Index into code memory using this pc and copy all instruction fields into
				 * fetch latch
				 */
				APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
				strcpy(cpu->stage[F].opcode, current_ins->opcode);
				cpu->stage[F].rd = current_ins->rd;
				cpu->stage[F].rs1 = current_ins->rs1;
				cpu->stage[F].rs2 = current_ins->rs2;
				cpu->stage[F].imm = current_ins->imm;
				cpu->stage[F].rd = current_ins->rd;

				/* Update PC for next instruction */
				cpu->pc += 4;
				alreadyFetched=1;
			}			
			cpu->stage[F].stalled =1;
		}
  }
  else
  {
	if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Decode/RF", stage);
    }
  }
  return 0;
}

/*
 *  Execute Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
execute(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[EX];
  if ((stage->pc != 0) && (strcmp(stage->opcode, "STORE") != 0) && 
	  (strcmp(stage->opcode, "BZ") != 0) && (strcmp(stage->opcode, "BNZ") != 0) && (strcmp(stage->opcode, "JUMP") != 0))
  {
	  cpu->regs_valid[stage->rd] = 1;
  }
  if (!stage->busy && !stage->stalled) {
	  
	if (mulEXtoMEM == 1) {
		cpu->stage[DRF].stalled=0;
		cpu->stage[F].stalled=0;
		/* Only incrementing stage pointer for Decode/RF stage*/
		cpu->stage[EX] = cpu->stage[DRF];
		/* Only incrementing stage pointer for Fetch stage*/
		cpu->stage[DRF] = cpu->stage[F];
		mulEXtoMEM=0;
    }
	
	/* BZ */
    if (strcmp(stage->opcode, "BZ") == 0) {
		if (zeroFlag == 1) {
			stage->buffer = integerALU(stage->pc,stage->imm);
			cpu->stage[F].stalled=1;
		}		
    }
	
	/* BNZ */
    if (strcmp(stage->opcode, "BNZ") == 0) {
		if (zeroFlag != 1) {
			stage->buffer = integerALU(stage->pc,stage->imm);
			cpu->stage[F].stalled=1;
		}		
    }
	
	/* JUMP */
    if (strcmp(stage->opcode, "JUMP") == 0) {
		if (zeroFlag != 1) {
			stage->buffer = integerALU(stage->rs1_value,stage->imm);
			cpu->stage[F].stalled=1;
		}		
    }
	
	/* Store */
    if (strcmp(stage->opcode, "STORE") == 0) {
		/* computing memory address */
		stage->buffer = integerALU(stage->rs2_value,stage->imm);
    }

    /* MOVC */
    if (strcmp(stage->opcode, "MOVC") == 0) {
		stage->buffer = integerALU(stage->imm, 0);
    }
	
	/* LOAD */
    if (strcmp(stage->opcode, "LOAD") == 0) {
		/* computing memory address */
		stage->buffer = integerALU(stage->rs1_value,stage->imm);
    }
	
	/* ADD */
    if (strcmp(stage->opcode, "ADD") == 0) {
		stage->buffer = integerALU(stage->rs1_value, stage->rs2_value);
    }
	
    /* SUB */
    if (strcmp(stage->opcode, "SUB") == 0) {
		stage->buffer = integerALU(stage->rs1_value, -stage->rs2_value);
    }
	
	/* MUL */
    if (strcmp(stage->opcode, "MUL") == 0) {
		mulCycleCounter++;
		if(strcmp(cpu->stage[MEM].opcode,"MUL") == 0)
		{
			mulCycleCounter=1;
		}
		if(mulCycleCounter == 1)
		{
			stage->buffer = mulALU(stage->rs1_value, stage->rs2_value);
			cpu->stage[MEM] = nop;
			print_stage_content("Execute", &cpu->stage[EX]);
			cpu->stage[DRF].stalled=1;
			
			/* Only fetching the instruction and not incrementing stage pointer */
			/* Store current PC in fetch latch */
			cpu->stage[F].pc = cpu->pc;

			/* Index into code memory using this pc and copy all instruction fields into
			 * fetch latch
			 */
			APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
			strcpy(cpu->stage[F].opcode, current_ins->opcode);
			cpu->stage[F].rd = current_ins->rd;
			cpu->stage[F].rs1 = current_ins->rs1;
			cpu->stage[F].rs2 = current_ins->rs2;
			cpu->stage[F].imm = current_ins->imm;
			cpu->stage[F].rd = current_ins->rd;

			/* Update PC for next instruction */
			cpu->pc += 4;
			cpu->stage[F].stalled=1;
		
			if ( ((strcmp(cpu->stage[DRF].opcode, "STORE") == 0 || strcmp(cpu->stage[DRF].opcode, "ADD") == 0 || strcmp(cpu->stage[DRF].opcode, "SUB") == 0 ||
			strcmp(cpu->stage[DRF].opcode, "AND") == 0 || strcmp(cpu->stage[DRF].opcode, "OR") == 0 || strcmp(cpu->stage[DRF].opcode, "EX-OR") == 0 ||
			strcmp(cpu->stage[DRF].opcode, "MUL") == 0) && (cpu->regs_valid[cpu->stage[DRF].rs1] != 1) && (cpu->regs_valid[cpu->stage[DRF].rs2] != 1))
			|| ((strcmp(cpu->stage[DRF].opcode, "LOAD") == 0 || strcmp(cpu->stage[DRF].opcode, "JUMP") == 0) && (cpu->regs_valid[cpu->stage[DRF].rs1] != 1)) )
			{
				/* Read data from register file */
				if (strcmp(cpu->stage[DRF].opcode, "STORE") == 0 || strcmp(cpu->stage[DRF].opcode, "ADD") == 0 || strcmp(cpu->stage[DRF].opcode, "SUB") == 0 ||
					strcmp(cpu->stage[DRF].opcode, "AND") == 0 || strcmp(cpu->stage[DRF].opcode, "OR") == 0 || strcmp(cpu->stage[DRF].opcode, "EX-OR") == 0 ||
					strcmp(cpu->stage[DRF].opcode, "MUL") == 0) {
					cpu->stage[DRF].rs1_value=cpu->regs[cpu->stage[DRF].rs1];
					cpu->stage[DRF].rs2_value=cpu->regs[cpu->stage[DRF].rs2];
				}
				
				/* Read data from register file for LOAD */
				if (strcmp(cpu->stage[DRF].opcode, "LOAD") == 0 || strcmp(cpu->stage[DRF].opcode, "JUMP") == 0) {
					cpu->stage[DRF].rs1_value=cpu->regs[cpu->stage[DRF].rs1];
				}
			}
			
			return 0;
		}
		if(mulCycleCounter == 2)
		{
			mulCycleCounter=0;
			mulEXtoMEM=1;
			cpu->stage[DRF].stalled=1;
			cpu->stage[F].stalled=1;
			print_stage_content("Execute", stage);
			cpu->stage[MEM] = cpu->stage[EX];
			cpu->stage[EX] = nop;
			
			if ( ((strcmp(cpu->stage[DRF].opcode, "STORE") == 0 || strcmp(cpu->stage[DRF].opcode, "ADD") == 0 || strcmp(cpu->stage[DRF].opcode, "SUB") == 0 ||
			strcmp(cpu->stage[DRF].opcode, "AND") == 0 || strcmp(cpu->stage[DRF].opcode, "OR") == 0 || strcmp(cpu->stage[DRF].opcode, "EX-OR") == 0 ||
			strcmp(cpu->stage[DRF].opcode, "MUL") == 0) && (cpu->regs_valid[cpu->stage[DRF].rs1] != 1) && (cpu->regs_valid[cpu->stage[DRF].rs2] != 1))
			|| ((strcmp(cpu->stage[DRF].opcode, "LOAD") == 0 || strcmp(cpu->stage[DRF].opcode, "JUMP") == 0) && (cpu->regs_valid[cpu->stage[DRF].rs1] != 1)) )
			{
				/* Read data from register file */
				if (strcmp(cpu->stage[DRF].opcode, "STORE") == 0 || strcmp(cpu->stage[DRF].opcode, "ADD") == 0 || strcmp(cpu->stage[DRF].opcode, "SUB") == 0 ||
					strcmp(cpu->stage[DRF].opcode, "AND") == 0 || strcmp(cpu->stage[DRF].opcode, "OR") == 0 || strcmp(cpu->stage[DRF].opcode, "EX-OR") == 0 ||
					strcmp(cpu->stage[DRF].opcode, "MUL") == 0) {
					cpu->stage[DRF].rs1_value=cpu->regs[cpu->stage[DRF].rs1];
					cpu->stage[DRF].rs2_value=cpu->regs[cpu->stage[DRF].rs2];
				}
				
				/* Read data from register file for LOAD */
				if (strcmp(cpu->stage[DRF].opcode, "LOAD") == 0 || strcmp(cpu->stage[DRF].opcode, "JUMP") == 0) {
					cpu->stage[DRF].rs1_value=cpu->regs[cpu->stage[DRF].rs1];
				}
			}
			
			return 0;
		}
    }
	
	/* AND */
    if (strcmp(stage->opcode, "AND") == 0) {
		stage->buffer = andALU(stage->rs1_value, stage->rs2_value);
    }
	
	/* OR */
    if (strcmp(stage->opcode, "OR") == 0) {
		stage->buffer = orALU(stage->rs1_value, stage->rs2_value);
    }
	
	/* EX-OR */
    if (strcmp(stage->opcode, "EX-OR") == 0) {
		stage->buffer = xorALU(stage->rs1_value, stage->rs2_value);
    }

    /* Copy data from Execute latch to Memory latch*/
    cpu->stage[MEM] = cpu->stage[EX];

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Execute", stage);
    }
		
    /* HALT */
    if (strcmp(stage->opcode, "HALT") == 0) {
		cpu->stage[DRF] =nop;
		cpu->stage[F] =nop;
		cpu->stage[DRF].stalled =1;
		cpu->stage[F].stalled =1;
    }
  if ((stage->pc != 0) && (strcmp(stage->opcode, "STORE") != 0) && 
	  (strcmp(stage->opcode, "BZ") != 0) && (strcmp(stage->opcode, "BNZ") != 0) && (strcmp(stage->opcode, "JUMP") != 0))
  {
	  cpu->regs_valid[stage->rd] = 1;
  }
  }
  return 0;
}

/*
 *  Memory Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
memory(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[MEM];  
  if (!stage->busy && !stage->stalled) {
    /* Store */
    if (strcmp(stage->opcode, "STORE") == 0) {
		cpu->data_memory[stage->buffer/4]=stage->rs1_value;
    }
	
	/* BZ */
    if (strcmp(stage->opcode, "BZ") == 0) {
		if (zeroFlag != 1) {
			stage->buffer = integerALU(stage->pc,stage->imm);
			cpu->pc = stage->buffer;	
			cpu->stage[EX]=nop;
			cpu->stage[DRF]=nop;
			cpu->stage[F].stalled=0;
		}		
    }

	/* BNZ */
    if (strcmp(stage->opcode, "BNZ") == 0) {
		if (zeroFlag != 1) {
			stage->buffer = integerALU(stage->pc,stage->imm);
			cpu->pc = stage->buffer;	
			cpu->stage[EX]=nop;
			cpu->stage[DRF]=nop;
			cpu->stage[F].stalled=0;
		}		
    }

	/* JUMP */
    if (strcmp(stage->opcode, "JUMP") == 0) {
		if (zeroFlag != 1) {
			stage->buffer = integerALU(stage->rs1_value,stage->imm);
			cpu->pc = stage->buffer;	
			cpu->stage[EX]=nop;
			cpu->stage[DRF]=nop;
			cpu->stage[F].stalled=0;
		}		
    }

    /* LOAD */
    if (strcmp(stage->opcode, "LOAD") == 0) {
		stage->buffer=cpu->data_memory[stage->buffer/4];
    }

    /* Copy data from decode latch to execute latch*/
    cpu->stage[WB] = cpu->stage[MEM];

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Memory", stage);
    }
	
	/* HALT */
    if (strcmp(stage->opcode, "HALT") == 0) {
		cpu->stage[EX] =nop;
		print_stage_content("Execute", &cpu->stage[EX]);
		cpu->stage[DRF] =nop;
		cpu->stage[F] =nop;
		cpu->stage[DRF].stalled =1;
		cpu->stage[F].stalled =1;
		cpu->stage[EX].stalled = 1;
    }
  }  
  return 0;
}

/*
 *  Writeback Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
writeback(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[WB];
  if (!stage->busy && !stage->stalled) {

    /* Update register file */
    if (strcmp(stage->opcode, "MOVC") == 0 || strcmp(stage->opcode, "ADD") == 0 || strcmp(stage->opcode, "SUB") == 0 || 
		strcmp(stage->opcode, "LOAD") == 0 || strcmp(stage->opcode, "MUL") == 0 || strcmp(stage->opcode, "AND") == 0 || 
		strcmp(stage->opcode, "EX-OR") == 0 || strcmp(stage->opcode, "OR") == 0 || strcmp(stage->opcode, "LOAD") == 0) {
      cpu->regs[stage->rd] = stage->buffer;
	  cpu->regs_valid[stage->rd] = 0;
    }
	
	/* Condition to stop simulation */
	if ( stage->pc == (4000+cpu->code_memory_size*4-4) || strcmp(stage->opcode, "HALT") == 0 )
	{
		stopSimulation = 1;
	}

    cpu->ins_completed++;

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Writeback", stage);
    }
	
	/* HALT */
    if (strcmp(stage->opcode, "HALT") == 0) {
		cpu->stage[MEM] =nop;
		print_stage_content("Memory", &cpu->stage[MEM]);
		cpu->stage[EX] =nop;
		print_stage_content("Execute", &cpu->stage[EX]);
		cpu->stage[DRF] =nop;
		cpu->stage[F] =nop;
		cpu->stage[DRF].stalled =1;
		cpu->stage[F].stalled =1;
		cpu->stage[EX].stalled = 1;
		cpu->stage[MEM].stalled = 1;
    }
	
  }
  return 0;
}

int integerALU(int input1, int input2)
{
	int result = input1 + input2;
	if (result == 0) {
		zeroFlag = 1;
	}
	else {
		zeroFlag = 0;
	}
	return result;
}

int mulALU(int input1, int input2)
{
	int result = input1 * input2;
	if (result == 0) {
		zeroFlag = 1;
	}
	else {
		zeroFlag = 0;
	}
	return result;
}

int andALU(int input1, int input2)
{
	int result = input1 & input2;
	if (result == 0) {
		zeroFlag = 1;
	}
	else {
		zeroFlag = 0;
	}
	return result;
}

int orALU(int input1, int input2)
{
	int result = input1 | input2;
	if (result == 0) {
		zeroFlag = 1;
	}
	else {
		zeroFlag = 0;
	}
	return result;
}

int xorALU(int input1, int input2)
{
	int result = input1 ^ input2;
	if (result == 0) {
		zeroFlag = 1;
	}
	else {
		zeroFlag = 0;
	}
	return result;
}

void printRegValues(APEX_CPU* cpu)
{
	printf("-------------------------------------------------\n");
    printf("------STATE OF ARCHITECTURAL REGISTER FILE-------\n");
    printf("-------------------------------------------------\n");
	for (int i=0;i<(int)(sizeof(cpu->regs)/sizeof(cpu->regs[0]));i++)
	{
		char valid_bit[20]="Valid";
		if(cpu->regs_valid[i] == 1)
		{
			strcpy( valid_bit, "Invalid"); 
		}
		printf("cpu->regs[%d] : %d\tcpu->regs_valid[%d] : %s\n",i,cpu->regs[i],i,valid_bit);
	}
}

void printMemoryData(APEX_CPU* cpu)
{
    printf("--------------------------------\n");
    printf("------STATE OF DATA MEMORY------\n");
    printf("--------------------------------\n");
	for (int i=0;i<100;i++)
	{
		printf("|\tMEM[%d] \t|Address : %d\t|\tData Value : %d\n",i,i*4,cpu->data_memory[i]);
	}
}

/*
 *  APEX CPU simulation loop
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
APEX_cpu_run(APEX_CPU* cpu, char* operation, int cycles)
{
	if (strcmp(operation,"display") == 0) {
		display=1;
	}
  while (1) {
	  
	if (stopSimulation == 1 || cpu->clock == cycles) {
      printf("(apex) >> Simulation Complete\n");
      break;
    }
	
    if (ENABLE_DEBUG_MESSAGES) {
      printf("--------------------------------\n");
      printf("Clock Cycle #: %d\n", cpu->clock+1);
      printf("--------------------------------\n");
    }
	
	writeback(cpu);
	memory(cpu);
	execute(cpu);
	decode(cpu);
    fetch(cpu);
	//printRegValues(cpu);
    cpu->clock++;

  }
  	printRegValues(cpu);
	printMemoryData(cpu);
  return 0;
}