#include <bits/stdc++.h>
#include <fstream>
using namespace std;

// defining all registers
enum
{
    R0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
};

uint8_t DCache[256]; // Data cache
uint8_t ICache[256]; // Instruction cache

// defining all buffers
struct IF_ID
{
    uint16_t instr; // Instruction buffer
} i_buffer;

struct ID_EX
{
    uint8_t opcode; // Operation code
    bool dependency, jump; // Dependency and jump flags
    uint8_t rd; // Destination register
    uint8_t A; // Source register A
    uint8_t B; // Source register B
    int8_t imm; // Immediate value
} d_buffer;

struct EX_MEM
{
    uint8_t opcode; // Operation code
    uint8_t rd; // Destination register
    uint8_t ALUOutput; // ALU output

} e_buffer;

struct MEM_WB
{
    uint8_t opcode; // Operation code
    uint8_t rd; // Destination register
    uint8_t LMD; // Load Memory Data
} m_buffer;

uint8_t reg[16] = {}; // Register file
int reg_occupied[16]={}; // Register occupancy status
uint8_t PC = 0x0; // Program Counter

// defining all opcodes
enum
{
    OP_ADD, /* add  */
    OP_SUB, /* subtract */
    OP_MUL, /* multiply */
    OP_INC, /* increment */
    OP_AND, /* bitwise and */
    OP_OR,  /* bitwise or */
    OP_XOR, /* bitwise xor */
    OP_NOT, /* bitwise not */

    OP_SLLI, /* shift left logical immediate */
    OP_SRLI, /* shift right logical immediate */
    OP_LI,   /* load indirect */
    OP_LD,   /* load */
    OP_ST,   /* store */
    OP_JMP,  /* jump */
    OP_BEQZ, /* branch if equal to zero */

    OP_HLT, /* halt */
};

int arth_instr = 0, logic_instr = 0, shift_instr = 0, mem_instr = 0,li_instr=0, control_instr = 0, halt_instr = 0;
int no_of_cycles = 0, no_of_stalls = 0, no_of_instructions = 0;
int data_stalls = 0, control_stalls = 0;
int running = 0;

bool Free[5], Skip[5];

ifstream ins_file("input/ICache.txt", ios::in); // Input file stream for instruction cache
ifstream data_file("input/DCache.txt", ios::in); // Input file stream for data cache
ofstream data_out("output/DCache.txt", ios::out); // Output file stream for data cache
ofstream ofile("output/Output.txt", ios::out); // Output file stream for program output

// Function to read register values from file
void read_reg()
{
    ifstream reg_file("input/RF.txt", ios::in); // Input file stream for register file
    for (int i = 0; i < 16; i++)
    {
        string a;
        reg_file >> a;
        uint8_t regv = stoi(a, 0, 16);
        reg[i] = regv;
        //cout<<hex<<"R"<<i<<": "<<(unsigned)reg<<endl;
        reg_occupied[i] = 0;
    }
    reg_file.close();
}

// Function to read instructions from ICache file
void get_ins()
{
    for(int i=0;i<256;i++)
    {
    string a;
    ins_file >> a;
    ICache[i]= stoi(a,0,16);
    }
    ins_file.close();
}

// Function to read data from DCache file
void get_data()
{
    for(int i=0;i<256;i++)
    {
    string a;
    data_file >> a;
    DCache[i]= stoi(a,0,16);
    }
    data_file.close();
}

// Function to write data to DCache file
void put_data()
{
    for(int i=0;i<256;i++)
    {
    data_out<<hex<<(unsigned)DCache[i]<<endl;
    }
}

// Function to read instruction from ICache
uint16_t ins_read(uint8_t address)
{
    uint8_t high = ICache[address];
    uint8_t low = ICache[address+1];
    
    uint16_t instr = (high << 8) | low;
    return instr;
}

// Function to read data from DCache
uint8_t data_read(uint8_t address)
{
    return DCache[address];
}

// Function to write data to DCache
void data_write(uint8_t address, uint8_t val)
{
    DCache[address] = val;
    return;
}

// Instruction Fetch stage
void ins_fetch()
{
    if (Skip[0] || !Free[0])
    {
        if (!d_buffer.dependency)
            Skip[1] = true;

        return;
    }
    running += 1;
    //cout<<"Fetch\n";
    i_buffer.instr = ins_read(PC);
    //cout<<hex<<"PC: "<<(unsigned)PC<<" Instruction: "<<hex<<i_buffer.instr<<endl;
    PC += 2;

    Skip[1] = false;
}

// Instruction Decode stage
void ins_decode()
{
    
    if (Skip[1] || !Free[1])
    {
        Skip[2] = true;
        return;
    }

    running += 1;
    //cout<<"Decode\n";
    d_buffer.opcode = i_buffer.instr >> 12;
    d_buffer.rd = (i_buffer.instr >> 8) & 0x000F;
    //cout<<hex<<"OPcode: "<<(unsigned)d_buffer.opcode<<" RD:"<<(unsigned)d_buffer.rd<<endl;
    d_buffer.dependency = false;
    d_buffer.jump = false;

    switch (d_buffer.opcode)
    {
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_AND:
    case OP_OR:
    case OP_XOR:
    {
        if (reg_occupied[(i_buffer.instr >> 4) & 0x000F] || reg_occupied[i_buffer.instr & 0x000F])
        {
            d_buffer.dependency = true;
        }
        else
        {
            reg_occupied[d_buffer.rd] +=1;
            d_buffer.A = reg[(i_buffer.instr >> 4) & 0x000F];
            d_buffer.B = reg[i_buffer.instr & 0x000F];
        }
        break;
    }
    case OP_NOT:
        if (reg_occupied[(i_buffer.instr >> 4) & 0x000F])
        {
            d_buffer.dependency = true;
        }
        else
        {
            reg_occupied[d_buffer.rd]+=1;
            d_buffer.A = reg[(i_buffer.instr >> 4) & 0x000F];
            d_buffer.B = 0;
        }
        break;

    case OP_SLLI:
    case OP_SRLI:
    case OP_LD:
    {
        if (reg_occupied[(i_buffer.instr >> 4) & 0x000F])
        {
            d_buffer.dependency = true;
        }
        else
        {
            reg_occupied[d_buffer.rd]+=1;
            d_buffer.A = reg[(i_buffer.instr >> 4) & 0x000F];
            d_buffer.imm = i_buffer.instr & 0x00FF;
        }
    }
    break;

    case OP_ST:
    {
        ////cout<<reg_occupied[(i_buffer.instr >> 4) & 0x000F]<<" ^ "<<unsigned((i_buffer.instr >> 4) & 0x000F)<<endl;
        if (reg_occupied[(i_buffer.instr >> 4) & 0x000F] || reg_occupied[d_buffer.rd])
        {
            d_buffer.dependency = true;
        }
        else
        {
            d_buffer.A = reg[(i_buffer.instr >> 4) & 0x000F];
            d_buffer.imm = i_buffer.instr & 0x00FF;
        }
    }
    break;

    case OP_INC:
        if (reg_occupied[d_buffer.rd])
            d_buffer.dependency = true;
        else
        {
            reg_occupied[d_buffer.rd]+=1;
            d_buffer.A = reg[d_buffer.rd];
            d_buffer.B = 0;
        }
        break;

    case OP_LI:
        reg_occupied[d_buffer.rd]+=1;
        d_buffer.imm = i_buffer.instr & 0x00FF;
        break;

    case OP_JMP:
        d_buffer.A = 0;
        d_buffer.B = 0;
        d_buffer.jump = true;
        d_buffer.imm = i_buffer.instr >> 4 & 0x00FF;
        break;

    case OP_BEQZ:
        if (reg_occupied[d_buffer.rd])
            d_buffer.dependency = true;
        else
        {
            d_buffer.jump = true;
            d_buffer.A = reg[d_buffer.rd];
            d_buffer.B = 0;
            d_buffer.imm = i_buffer.instr & 0x00FF;
            //cout<<(unsigned)d_buffer.A<<" %\n";
        }
        break;

    case OP_HLT:
    {
        Skip[0] = true;
    }
    break;
    }

    if (d_buffer.dependency)
    {
        data_stalls += 1;
        Free[0] = false;
        Skip[2] = true;
    }
    else if (d_buffer.jump)
    {
        control_stalls += 2;
        //skip the next fetch and decode stages and go to execute stage
        Free[0] = false;
        Skip[1] = true;
        Skip[2] = false;
    }
    else
    {
        Skip[2] = false;
    }
}

// ALU function to perform arithmetic and logical operations
int ALU(uint8_t opcode, uint8_t a, uint8_t b)
{
    //cout<<"ALU\n";
    switch (opcode)
    {
    case OP_ADD:
    case OP_LD:
    case OP_ST:
        return a + b;
    case OP_SUB:
        return a - b;
    case OP_MUL:
        return a * b;
    case OP_INC:
        return a + 1;
    case OP_AND:
        return a & b;
    case OP_OR:
        return a | b;
    case OP_XOR:
        return a ^ b;
    case OP_NOT:
        return ~a;
    case OP_SLLI:
        return a << b;
    case OP_SRLI:
        return a >> b;

    case OP_BEQZ:
        if (unsigned(a) == 0)
            return PC + 2 * b;
        else
            return PC;
    default:
        return 0;
    }
}

// Instruction Execute stage
void ins_execute()
{
    
    if (Skip[2] || !Free[2])
    {
        Skip[3] = true;
        return;
    }

    running += 1;
    Skip[3] = false;
    //cout<<"Execute\n";
    e_buffer.opcode = d_buffer.opcode;

    switch (e_buffer.opcode)
    {
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
        arth_instr++;
        e_buffer.ALUOutput = ALU(e_buffer.opcode, d_buffer.A, d_buffer.B);
        e_buffer.rd = d_buffer.rd;
        break;

    case OP_AND:
    case OP_OR:
    case OP_XOR:
        logic_instr++;
        e_buffer.ALUOutput = ALU(e_buffer.opcode, d_buffer.A, d_buffer.B);
        e_buffer.rd = d_buffer.rd;
        break;

    case OP_NOT:
        logic_instr++;
        e_buffer.ALUOutput = ALU(e_buffer.opcode, d_buffer.A, 0);
        e_buffer.rd = d_buffer.rd;
        break;

    case OP_INC:
        arth_instr++;
        e_buffer.ALUOutput = ALU(e_buffer.opcode, d_buffer.A, 0);
        e_buffer.rd = d_buffer.rd;
        break;

    case OP_SLLI:
    case OP_SRLI:
        shift_instr++;
        e_buffer.ALUOutput = ALU(e_buffer.opcode, d_buffer.A, d_buffer.imm);
        e_buffer.rd = d_buffer.rd;
        break;

    case OP_LD:
    case OP_ST:
        mem_instr++;
        e_buffer.ALUOutput = ALU(e_buffer.opcode, d_buffer.A, d_buffer.imm);
        e_buffer.rd = d_buffer.rd;
        break;

    case OP_LI:
        li_instr++;
        e_buffer.ALUOutput = d_buffer.imm;
        e_buffer.rd = d_buffer.rd;
        break;

    case OP_JMP:
        control_instr++;
        e_buffer.ALUOutput= d_buffer.imm * 2;
        Free[0] = false;
        Free[1] = false;
        break;

    case OP_BEQZ:
        control_instr++;
        e_buffer.ALUOutput = ALU(e_buffer.opcode, d_buffer.A, d_buffer.imm);
        Free[0] = false;
        Free[1] = false;
        break;

    case OP_HLT:
        halt_instr++;
        break;
    default:
        break;
    }
    return;
}

// Memory Access stage
void mem_access()
{
    
    if (Skip[3] || !Free[3])
    {
        Skip[4] = true;
        return;
    }

    m_buffer.opcode = e_buffer.opcode;
    Skip[4] = false;
    running += 1;
    //cout<<"Memory\n";
    if (m_buffer.opcode == OP_HLT)
        return;
    if(m_buffer.opcode == OP_JMP)
    {
        PC += e_buffer.ALUOutput;
        return;
    }
    if(m_buffer.opcode == OP_BEQZ)
    {
            PC = e_buffer.ALUOutput;
            return;
    }
    if (m_buffer.opcode == OP_LD)
    {
        m_buffer.LMD = data_read(e_buffer.ALUOutput);
        m_buffer.rd = e_buffer.rd;
    }
    else if (m_buffer.opcode == OP_ST)
    {
        data_write(e_buffer.ALUOutput, reg[e_buffer.rd]);
    }
    else
    {
        m_buffer.rd = e_buffer.rd;
        m_buffer.LMD = e_buffer.ALUOutput;
    }
}

// Write Back stage
void write_back()
{
    
    if (Skip[4] || !Free[4])
    {
        return;
    }
    running += 1;
    
    int opcode = m_buffer.opcode;

    if (opcode == OP_HLT)
        return;

    if (m_buffer.opcode == OP_LD)
    {
        reg[m_buffer.rd] = m_buffer.LMD;
        reg_occupied[m_buffer.rd] -=1;
    }
    else if (m_buffer.opcode == OP_ST || m_buffer.opcode == OP_JMP || m_buffer.opcode == OP_BEQZ)
    {
    }
    else
    {
        reg[m_buffer.rd] = m_buffer.LMD;
        reg_occupied[m_buffer.rd] -= 1;
        //cout<<hex<<"RD: "<<unsigned(m_buffer.rd)<<" Value: "<<unsigned(m_buffer.LMD)<<endl;
    }

    return;
}

int main()
{
    get_ins();
    get_data();
    read_reg();
    
    Free[0] = true;
    Skip[1] = false;
    PC=0x0;
    for (int i = 1; i < 5; i++)
    {
        Free[i] = false;
        Skip[i] = true;
    }

    running = 1;
    
    while (running)
    {
        running = 0;
        write_back();
        mem_access();
        ins_execute();
        ins_decode();
        ins_fetch();

        for (int i = 0; i < 5; i++)
        {
            Free[i] = true;
        }

        if (running)
        {
            no_of_cycles += 1;
            //cout<<"Cycle: "<<no_of_cycles<<endl;
        }
    }
    put_data();
    ofile << "Total number of instructions executed: " << arth_instr + logic_instr + shift_instr + li_instr +mem_instr + control_instr + halt_instr << endl;
    ofile << "Number of instructions in each class" << endl;
    ofile << "Arithmetic instructions: " << arth_instr << endl;
    ofile << "Logical instructions: " << logic_instr << endl;
    ofile << "Shift instructions: " << shift_instr << endl;
    ofile << "Memory instructions: " << mem_instr << endl;
    ofile << "Load immediate instructions: " << li_instr <<endl;
    ofile << "Control instructions: " << control_instr << endl;
    ofile << "Halt instructions: " << halt_instr << endl;

    ofile << "Cycles Per Instruction: " << (double)no_of_cycles / (arth_instr + logic_instr + shift_instr + li_instr + mem_instr + control_instr + halt_instr) << endl;
    ofile << "Total number of stalls: " << data_stalls + control_stalls << endl;
    ofile << "Data stalls (RAW): " << data_stalls << endl;
    ofile << "Control stalls: " << control_stalls << endl;

    for (int i = 0; i < 16; i++)
    {
        //cout << "R" << i << ": " <<(unsigned) reg[i] << endl;
    }
}
