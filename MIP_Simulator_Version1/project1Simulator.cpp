/*---------------------------------------------------------------------------/
/File Name: project1Simulator.cpp
/Author:    Muhammed Muiktar
/Date:      3/26/2021
/Section:   02
/Email:     mmuktar1@umbc.edu
/
/Description: This program simulates MIPS pipline instructions
/             (No cache, no hazards, single EX stage, no branching)
/
/---------------------------------------------------------------------------*/

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <tuple>
#include <iomanip>

using namespace std;

// Vector used to store opcodes and functions for instructions
typedef vector<tuple<string, string, string>> R_instructions;
R_instructions R_INSTRUCTION_LIST;

typedef vector<tuple<string, string>> I_instructions;
I_instructions I_INSTRUCTION_LIST;

// Vector represents register names with their index
typedef vector<tuple<string, int>> registerIndex;
registerIndex LIST_REG_INDEX;

// Vector to store data memory and instruction memory
vector<string> list_0x00;
vector<string> list_0x100;

// vector to track stage clocks
vector<int> trackIFClock;
vector<int> trackIDClock;
vector<int> trackEXClock;
vector<int> trackMEMClock;
vector<int> trackWBClock;

// registers stored in array
int registers[32];

// index indicating index of register
const int INDEX_V = 2, INDEX_A = 4, INDEX_T0 = 8, INDEX_S = 16
                    , INDEX_T8 = 24, INDEX_K = 26;

// represents pipline stages with numbers
const int FETCH = 1, DECODE = 2, EXECUTE = 3, ACCESS = 4, WRITE_BACK = 5;

int PC = 0;
int clockCycle = 1;
string outputFile;

// vector to queue stages for pipline
typedef vector<tuple<int, int>> Queue;
Queue queueInstructions;

// Precondition:     N/A
// Post condition:   Parses instruction file inserting into instruction memory
void loadInstruction(string filename);

// Precondition:     N/A
// Post condition:   Stores data file into data memory
void loadData(string filename);

// Precondition:     N/A
// Post condition:   Creates file with instructions and clock cycle at each stage
void writeOutput(string filename);

// Precondition:     Instruction vectors are initalized
// Post condition:   Instruction vectors are populated with opcodes and funct for
//                   the instructions
void loadInstrucionList();

// Precondition:     Register list is initalized
// Post condition:   Popluates registers
void loadRegister();

// Precondition:     Register list is initalized and popluated from instruction
// Post condition:   Creates a txt with register values
void writeRegisterValue();

// Precondition:     Data list is initalized and updated from instruction
// Post condition:   Creates a txt with updated data
void writeDataValue();

//-----------------------------------------------------------------------------

// Precondition:     LIST_REG_INDEX is poplutated
// Post condition:   Returns the index of a given register as it correlates with
//                   the register's index in the register array
int getRegIndex(string reg);

// Precondition:     N/A
// Post condition:   Returns binary number with the given number of bits
string decimalToBinaryStr(int number, int numBits);

//-----------------------------------------------------------------------------

// Precondition:     N/A
// Post condition:   Instructions are ran in pipline order
void runInstructions();

// Precondition:     Instrution memory is populated
// Post condition:   Fetches instructions and populates registers
void instructionFetch();

// Precondition:     Instruction was fetched
// Post condition:   Decodes instructions and populates registers
void instructionDecode();

// Precondition:     Instruction was decoded
// Post condition:   Executes instructions and populates registers
void execute();

// Precondition:     Instrucion was executed
// Post condition:   Access memory for intruction and populates registers
void accessMemory();

// Precondition:     Instrucion memory accessed
// Post condition:   Writes to register in instructions and populates registers
void writeBack();

//-----------------------------------------------------------------------------

// Precondition:     N/A
// Post condition:   Poplates id_ex registers with proper alu controls
void makeControl(string opcode);

// Precondition:     N/A
// Post condition:   Returns operations with given variables
string ALU(string var1, string var2, string control);

// Precondition:     N/A
// Post condition:   Returns alu control given op code and funct
string ALUcontrol(string aluOp, string funct, string opcode);

// Precondition:     N/A
// Post condition:   Popluates instruction queue with respect to current clock cycle
void insertQueue();

// IF_ID Register
struct IF_ID
{

    string fetchedInstruction;
    bool stopInstrucions = false;

} if_idReg;

// ID_EX Register
struct ID_EX
{

    string readData1, readData2;
    string instrutction15_0;
    string instrutction20_16;
    string instrutction15_11;
    string opcode;

    // Controls
    bool aluSrc;
    string aluOp;
    bool regDst;

    bool memWrite;
    bool memRead;

    bool regWrite;
    bool memToReg;

} id_exReg;

// EXE_MEM Register
struct EX_MEM
{

    string aluResult;
    string readData2;
    string regDestination;

    // Controls
    bool memWrite;
    bool memRead;

    bool regWrite;
    bool memToReg;

} ex_memReg;

// MEM_WB Registers
struct MEM_WB
{

    string readDataMem;
    string aluResult;
    string regDestination;

    // Controls
    bool regWrite;
    bool memToReg;

} mem_wbReg;

int main(int argc, char *argv[])
{

    try
    {

        // Number of arguments are not correct, needs four to execute program
        if (argc != 4)
        {

            throw "Format must be simulatorV1 <instructions> <data> <output>";
        }

        // Loading instructions and registers
        loadInstrucionList();
        loadRegister();

        // Parsing instructions
        loadInstruction(argv[1]);

        // Populating data memory
        loadData(argv[2]);

        // Runs Pipline
        runInstructions();

        outputFile = argv[3];

        // project output is written to files
        writeOutput(argv[1]);
        writeRegisterValue();
        writeDataValue();
    }
    catch (const char *error)
    {

        cout << "Error: " << error << endl;
        cout << "Example: inst.txt data.txt output.txt" << endl;
    }
}

void loadInstrucionList()
{

    // Insert R type instructions, instruction - opcode - funct
    R_INSTRUCTION_LIST.push_back(tuple<string, string, string>("ADD", "000000", "100000"));
    R_INSTRUCTION_LIST.push_back(tuple<string, string, string>("SUB", "000000", "100010"));
    R_INSTRUCTION_LIST.push_back(tuple<string, string, string>("AND", "000000", "100100"));
    R_INSTRUCTION_LIST.push_back(tuple<string, string, string>("OR", "000000", "100101"));
    R_INSTRUCTION_LIST.push_back(tuple<string, string, string>("SLL", "000000", "000000"));
    R_INSTRUCTION_LIST.push_back(tuple<string, string, string>("SRL", "000000", "000010"));

    // Insert I type instructions, instruction - opcode
    I_INSTRUCTION_LIST.push_back(tuple<string, string>("LW", "100011"));
    I_INSTRUCTION_LIST.push_back(tuple<string, string>("SW", "101011"));
    I_INSTRUCTION_LIST.push_back(tuple<string, string>("LI", "001000"));
    I_INSTRUCTION_LIST.push_back(tuple<string, string>("ADDI", "001000"));
    I_INSTRUCTION_LIST.push_back(tuple<string, string>("SUBI", "100001"));
    I_INSTRUCTION_LIST.push_back(tuple<string, string>("ANDI", "001100"));
    I_INSTRUCTION_LIST.push_back(tuple<string, string>("ORI", "001101"));
}

void loadRegister()
{

    // Inserts needed register names to represent index
    LIST_REG_INDEX.push_back(tuple<string, int>("$zero", 0));
    registers[0] = 0;

    LIST_REG_INDEX.push_back(tuple<string, int>("$at", 1));
    LIST_REG_INDEX.push_back(tuple<string, int>("$gp", 28));
    LIST_REG_INDEX.push_back(tuple<string, int>("$sp", 29));
    LIST_REG_INDEX.push_back(tuple<string, int>("$fp", 30));
    LIST_REG_INDEX.push_back(tuple<string, int>("$ra", 31));
}

int getRegIndex(string reg)
{

    for (vector<tuple<string, int>>::iterator i = LIST_REG_INDEX.begin(); i != LIST_REG_INDEX.end(); ++i)
    {

        // if register is found
        if (reg == get<0>(*i))
        {

            // return index
            return get<1>(*i);
        }
    }

    // gets the number from register's thrid index
    // example $s8, gets 8 and converts to int
    int addIndex = (int)reg[2] - 48;

    int index = 0;

    // finding index that represents the register integer
    if (reg[1] == 'v')
    {

        index = INDEX_V;
    }
    else if (reg[1] == 'a')
    {

        index = INDEX_A;
    }
    else if (reg[1] == 't')
    {

        // checks for temporary's integer
        if (addIndex >= 8)
        {
            addIndex -= 8;
            index = INDEX_T8;
        }
        else
            index = INDEX_T0;
    }
    else if (reg[1] == 's')
    {

        index = INDEX_S;
    }
    else if (reg[1] == 'k')
    {

        index = INDEX_K;
    }

    // returns computed index
    return (index + addIndex);
}

string decimalToBinaryStr(int number, int numBits)
{

    string makeBinary = "";
    int bit = 0;

    // while number can be divided by 2
    while (number > 0)
    {

        bit = number % 2;
        number /= 2;

        // makes binary
        makeBinary.insert(0, to_string(bit));
    }

    // makes sure binary has the corrct number of bits
    if (int(makeBinary.size()) > numBits)
    {

        makeBinary.resize(numBits);
    }
    else if (int(makeBinary.size()) < numBits)
    {

        while (int(makeBinary.size()) != numBits)
            makeBinary.insert(0, "0");
    }

    return makeBinary;
}

void loadInstruction(string filename)
{

    ifstream loadInst;
    loadInst.open(filename);

    // file does not exist
    if (loadInst.fail())
    {

        cerr << "Invalid instructions file " << filename << endl;
        exit(0);
    }

    string getInst;
    string getReg1, getReg2, getReg3, getCons, getAddr;
    string shamt;

    string temp, tempInst;
    string tempBinary = "";

    // while there is something to read
    while (loadInst >> getInst)
    {

        // checks for labels
        if (getInst[getInst.size() - 1] == 58)
        {

            // skips over labels not needed for now
            loadInst >> getInst;
        }

        // converts getInst to uppercase
        for (int i = 0; i < int(getInst.length()); i++)
        {

            tempInst.push_back(toupper(getInst[i]));
        }

        getInst = tempInst;
        tempInst = "";

        // checks instruction in R_INSTRUCTIONS
        for (vector<tuple<string, string, string>>::iterator i = R_INSTRUCTION_LIST.begin(); i != R_INSTRUCTION_LIST.end(); ++i)
        {

            if (getInst == get<0>(*i))
            {

                // adds opcode
                tempBinary.append(get<1>(*i));

                // gets register1
                loadInst >> getReg1;
                getReg1.replace(getReg1.size() - 1, 1, "");

                // gets register2
                loadInst >> getReg2;
                getReg2.replace(getReg2.size() - 1, 1, "");

                if (getInst == "SLL" || getInst == "SRL")
                {

                    // gets shamt for sll and srl
                    loadInst >> getCons;

                    // checks for hex
                    if (getCons[getCons.size() - 1] == 'h')
                    {

                        // converts hex to binary
                        getCons.replace(getCons.size() - 1, 1, "");
                        getCons.insert(0, "0x");

                        shamt = decimalToBinaryStr(stoi(getCons, nullptr, 16), 5);
                    }
                    else
                    {

                        // converts decimal to binary
                        shamt = decimalToBinaryStr(stoi(getCons), 5);
                    }

                    // adds rs rt and rd for srl and sll
                    tempBinary.append("00000");
                    tempBinary.append(decimalToBinaryStr(getRegIndex(getReg2), 5));
                    tempBinary.append(decimalToBinaryStr(getRegIndex(getReg1), 5));
                }
                else
                {

                    loadInst >> getReg3;

                    // adds rs rt and rd
                    tempBinary.append(decimalToBinaryStr(getRegIndex(getReg2), 5));
                    tempBinary.append(decimalToBinaryStr(getRegIndex(getReg3), 5));
                    tempBinary.append(decimalToBinaryStr(getRegIndex(getReg1), 5));

                    shamt = "00000";
                }

                // adds shamt and funct
                tempBinary.append(shamt);
                tempBinary.append(get<2>(*i));
            }
        }

        // checks instruction in I_INSTRUCTIONS
        for (vector<tuple<string, string>>::iterator i = I_INSTRUCTION_LIST.begin(); i != I_INSTRUCTION_LIST.end(); ++i)
        {

            if (getInst == get<0>(*i))
            {

                // adds opcode
                tempBinary.append(get<1>(*i));

                // gets register1
                loadInst >> getReg1;
                getReg1.replace(getReg1.size() - 1, 1, "");

                // if instruction is li
                if (getInst == "LI")
                {

                    getReg2 = "00000";
                }
                // if instructions is lw or sw
                else if (getInst == "LW" || getInst == "SW")
                {

                    loadInst >> temp;

                    int first_paren = temp.find("(");
                    int second_paren = temp.find(")");

                    // gets register2 for lw or sw
                    getReg2 = temp.substr(first_paren + 1, (second_paren - (first_paren + 1)));
                }
                else
                {

                    // gets register2
                    loadInst >> getReg2;
                    getReg2.replace(getReg2.size() - 1, 1, "");
                }

                if (getInst == "LW" || getInst == "SW")
                    // gets constant for lw or sw
                    getCons = temp.substr(0, temp.find("("));

                else
                    // gets constant/address
                    loadInst >> getCons;

                // adds rs rt
                tempBinary.append(decimalToBinaryStr(getRegIndex(getReg2), 5));
                tempBinary.append(decimalToBinaryStr(getRegIndex(getReg1), 5));

                // check if hex
                if (getCons[getCons.size() - 1] == 'h')
                {

                    // converts to hex
                    getCons.replace(getCons.size() - 1, 1, "");
                    getCons.insert(0, "0x");

                    // adds constant
                    tempBinary.append(decimalToBinaryStr(stoi(getCons, nullptr, 16), 16));
                }
                else
                {

                    // adds constant
                    tempBinary.append(decimalToBinaryStr(stoi(getCons), 16));
                }
            }
        }

        // if instruction is to halt
        if (getInst == "HLT")
            list_0x00.push_back("11111100000000000000000000000000");
        else
            list_0x00.push_back(tempBinary);

        tempBinary = "";
    }

    loadInst.close();
}

void loadData(string filename)
{

    ifstream loadData;
    loadData.open(filename);

    // if file does not exist
    if (loadData.fail())
    {

        cerr << "Invalid data file " << filename << endl;
        exit(0);
    }

    string getData;

    // while not at end of file
    while (loadData >> getData)
    {

        // adds data to data memory
        list_0x100.push_back(getData);
    }

    loadData.close();
}

void writeOutput(string filename)
{

    ifstream loadInst;
    loadInst.open(filename);

    ofstream outInst(outputFile);

    // file does not exist
    if (loadInst.fail())
    {

        cerr << "Invalid instructions file " << filename << endl;
        exit(0);
    }

    string getFirst, getInst, temp;
    int index = 0;

    // header
    outInst.width(58);
    outInst << right << "IF   ID   EX   MEM  WB";
    outInst << "\n";

    // while there are instructions to read
    while (loadInst >> getFirst)
    {

        // checks for label
        if (getFirst[getFirst.size() - 1] == 58)
        {

            // insert label with instruction
            outInst.width(10);
            outInst << left << getFirst;

            loadInst >> getFirst;
            outInst.width(10);
            outInst << left << getFirst;
        }
        else
        {

            // insert instruction
            outInst.width(10);
            outInst << left << "";
            outInst.width(10);
            outInst << left << getFirst;
        }

        // converts to uppercase
        for (int i = 0; i < int(getFirst.length()); i++)
        {

            temp.push_back(toupper(getFirst[i]));
        }

        getFirst = temp;
        temp = "";

        // checks end instruction call
        if (getFirst == "HLT")
        {

            // inserts spacing
            outInst.width(16);
            outInst << left << "";
        }
        else
        {

            // load register1
            loadInst >> getInst;
            outInst.width(5);
            outInst << left << getInst;

            // gets next part of instruction
            loadInst >> getInst;

            // checks specific instruction call
            if (getFirst == "LW" || getFirst == "SW" || getFirst == "LI")
            {

                outInst.width(11);
                outInst << left << getInst;
            }
            else
            {

                // inserts next two instrucions
                outInst.width(5);
                outInst << left << getInst;

                loadInst >> getInst;

                outInst.width(6);
                outInst << left << getInst;
            }
        }

        // inserts clock cycle from each stage
        if (index < int(trackIFClock.size()))
        {
            outInst.width(5);
            outInst << left << trackIFClock.at(index);
        }
        if (index < int(trackIDClock.size()))
        {
            outInst.width(5);
            outInst << left << trackIDClock.at(index);
        }
        if (index < int(trackEXClock.size()))
        {
            outInst.width(5);
            outInst << left << trackEXClock.at(index);
        }
        if (index < int(trackMEMClock.size()))
        {
            outInst.width(5);
            outInst << left << trackMEMClock.at(index);
        }
        if (index < int(trackWBClock.size()))
        {
            outInst.width(5);
            outInst << left << trackWBClock.at(index);
        }

        index++;
        outInst << "\n";
    }

    loadInst.close();
    outInst.close();
}

void writeRegisterValue()
{

    ofstream outReg("register.txt");

    // writes register values
    for (int i = 0; i < 32; i++)
    {

        outReg << "%" << i << " = "
               << decimalToBinaryStr(registers[i], 32) << endl;
    }
}

void writeDataValue()
{

    ofstream outData("dataUpdate.txt");

    // writes data memory
    for (int i = 0; i < int(list_0x100.size()); i++)
    {

        outData << list_0x100.at(i) << endl;
    }
}

void insertQueue()
{

    // inserts functions queue with respect to clock cycle
    queueInstructions.push_back(tuple<int, int>(FETCH, clockCycle));
    queueInstructions.push_back(tuple<int, int>(DECODE, clockCycle + 1));
    queueInstructions.push_back(tuple<int, int>(EXECUTE, clockCycle + 2));
    queueInstructions.push_back(tuple<int, int>(ACCESS, clockCycle + 3));
    queueInstructions.push_back(tuple<int, int>(WRITE_BACK, clockCycle + 4));
}

void runInstructions()
{

    // starts queue
    insertQueue();

    // while instruct is not halted
    while (!if_idReg.stopInstrucions)
    {

        // loops through queue
        for (vector<tuple<int, int>>::iterator i = queueInstructions.begin(); i != queueInstructions.end(); ++i)
        {

            // checks if stage needs to be executed given clock cycle
            if (get<1>(*i) == clockCycle)
            {

                // finds stage to run
                if (get<0>(*i) == FETCH)
                {
                    instructionFetch();
                }
                else if (get<0>(*i) == DECODE)
                {
                    instructionDecode();
                }
                else if (get<0>(*i) == EXECUTE)
                {
                    execute();
                }
                else if (get<0>(*i) == ACCESS)
                {
                    accessMemory();
                }
                else if (get<0>(*i) == WRITE_BACK)
                {
                    writeBack();
                }

                // removes stage
                queueInstructions.erase(i);
                i--;
            }
        }

        clockCycle++;

        // stops queue
        if (!if_idReg.stopInstrucions)
            insertQueue();
    }
}

void instructionFetch()
{

    // stop fetching when HLT is called in decode
    if (!if_idReg.stopInstrucions)
    {

        // gets instruction memory
        if_idReg.fetchedInstruction = list_0x00[PC];

        // updates program counter
        PC++;
    }

    trackIFClock.push_back(clockCycle);
}

void instructionDecode()
{

    // updates controls id_exReg
    string opcode = if_idReg.fetchedInstruction.substr(0, 6);

    // checks if halt
    if (opcode == "111111")
    {

        if_idReg.stopInstrucions = true;
    }
    else
    {

        // populates control values in id_ex register
        makeControl(opcode);

        string getFunct = if_idReg.fetchedInstruction.substr(26, 6);

        string register1;

        // if srl or sll
        if ((opcode == "000000") && (getFunct == "000000" || getFunct == "000010"))
        {

            // gets shamt for SRL and SLL
            register1 = if_idReg.fetchedInstruction.substr(21, 5);
            id_exReg.readData1 = register1;
        }
        else
        {

            // gets rs
            register1 = if_idReg.fetchedInstruction.substr(6, 5);
            id_exReg.readData1 = decimalToBinaryStr(registers[stoi(register1, nullptr, 2)], 32);
        }

        // gets rt
        string register2 = if_idReg.fetchedInstruction.substr(11, 5);
        id_exReg.readData2 = decimalToBinaryStr(registers[stoi(register2, nullptr, 2)], 32);

        // gets rd in R format
        id_exReg.instrutction15_11 = if_idReg.fetchedInstruction.substr(16, 5);

        // rd in I format
        id_exReg.instrutction20_16 = if_idReg.fetchedInstruction.substr(11, 5);

        string tempString = if_idReg.fetchedInstruction.substr(16, 16);
        int tempConst;

        // if lw or sw
        if (opcode == "100011" || opcode == "101011")
        {

            // makes up word difference in address
            tempConst = stoi(tempString, nullptr, 2);
            tempConst /= 4;
            id_exReg.instrutction15_0 = decimalToBinaryStr(tempConst, 16);
        }
        else
        {

            // gets last 16 bits for constant/address
            id_exReg.instrutction15_0 = if_idReg.fetchedInstruction.substr(16, 16);
        }

        // shifts constant/address to 32 bits
        while (int(id_exReg.instrutction15_0.size()) != 32)
            id_exReg.instrutction15_0.insert(0, "0");

        id_exReg.opcode = opcode;
    }

    trackIDClock.push_back(clockCycle);
}
void execute()
{

    string getFunct = id_exReg.instrutction15_0.substr(26, 6);

    // gets alu control
    string getAluControl = ALUcontrol(id_exReg.aluOp, getFunct, id_exReg.opcode);
    string getALUResult;

    // check alusrc and gets result
    if (id_exReg.aluSrc)
        getALUResult = ALU(id_exReg.readData1, id_exReg.instrutction15_0, getAluControl);

    else
        getALUResult = ALU(id_exReg.readData1, id_exReg.readData2, getAluControl);

    // checks register destination to exe_mem register
    if (id_exReg.regDst)
        ex_memReg.regDestination = id_exReg.instrutction15_11;
    else
        ex_memReg.regDestination = id_exReg.instrutction20_16;

    // passes to exe_mem register
    ex_memReg.aluResult = getALUResult;
    ex_memReg.readData2 = id_exReg.readData2;

    ex_memReg.memWrite = id_exReg.memWrite;
    ex_memReg.memRead = id_exReg.memRead;

    ex_memReg.regWrite = id_exReg.regWrite;
    ex_memReg.memToReg = id_exReg.memToReg;

    trackEXClock.push_back(clockCycle);
}
void accessMemory()
{

    string getAddress = ex_memReg.aluResult;
    string getWriteData = ex_memReg.readData2;

    mem_wbReg.readDataMem = "";

    // checks to write to memory
    if (ex_memReg.memWrite)

        // changes data, makes up for index at 0x100
        list_0x100[stoi(getAddress, nullptr, 2) - 256] = getWriteData;

    // checks to read from address
    if (ex_memReg.memRead)

        // passes whats read to mem_wb register
        mem_wbReg.readDataMem = list_0x100[stoi(getAddress, nullptr, 2) - 256];

    // passes to mem_wb register
    mem_wbReg.aluResult = ex_memReg.aluResult;
    mem_wbReg.regDestination = ex_memReg.regDestination;

    mem_wbReg.regWrite = ex_memReg.regWrite;
    mem_wbReg.memToReg = ex_memReg.memToReg;

    trackMEMClock.push_back(clockCycle);
}
void writeBack()
{

    string writeRegister = mem_wbReg.regDestination;
    string writeData;

    // checks to write register from memory
    if (mem_wbReg.memToReg)
        writeData = mem_wbReg.readDataMem;

    else
        writeData = mem_wbReg.aluResult;

    // checks to write to register
    if (mem_wbReg.regWrite)
        registers[stoi(writeRegister, nullptr, 2)] = stoi(writeData, nullptr, 2);

    trackWBClock.push_back(clockCycle);
}

void makeControl(string opcode)
{

    // r-type opcode
    if (opcode == "000000")
    {

        id_exReg.regDst = 1;
        id_exReg.aluSrc = 0;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;

        id_exReg.aluOp = "10";
    }
    // andi
    else if (opcode == "001100")
    {

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;

        id_exReg.aluOp = "11";
    }
    // ori
    else if (opcode == "001101")
    {

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;

        id_exReg.aluOp = "11";
    }
    // addi opcode
    else if (opcode == "001000")
    {

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;

        id_exReg.aluOp = "00";
    }
    // subi opcode (made up)
    else if (opcode == "100001")
    {

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;

        id_exReg.aluOp = "01";
    }
    // lw opcode
    else if (opcode == "100011")
    {

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 1;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 1;
        id_exReg.memWrite = 0;

        id_exReg.aluOp = "00";
    }
    // sw opcode
    else if (opcode == "101011")
    {

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 1;
        id_exReg.regWrite = 0;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 1;

        id_exReg.aluOp = "00";
    }
}

string ALU(string var1, string var2, string control)
{

    // converts from binary to int
    int num_var1 = stoi(var1, nullptr, 2);
    int num_var2 = stoi(var2, nullptr, 2);

    // set to zero to prevent code from breaking
    int result = 0;

    // checks alu control to do alu operation
    if (control == "0010")
    {
        result = num_var1 + num_var2;
    }
    else if (control == "0110")
    {
        result = num_var1 - num_var2;
    }
    else if (control == "0000")
    {
        result = num_var1 & num_var2;
    }
    else if (control == "0001")
    {
        result = num_var1 | num_var2;
    }
    else if (control == "1111")
    {
        result = num_var2 << num_var1;
    }
    else if (control == "1001")
    {
        result = num_var2 >> num_var1;
    }

    // returns a 32 bit answer
    return decimalToBinaryStr(result, 32);
}

string ALUcontrol(string aluOp, string funct, string opcode)
{

    // r-type instruction
    if (aluOp == "10")
    {

        // add control
        if (funct == "100000")
        {
            return "0010";
        }
        // subtract control
        else if (funct == "100010")
        {
            return "0110";
        }
        // and control
        else if (funct == "100100")
        {
            return "0000";
        }
        // or control
        else if (funct == "100101")
        {
            return "0001";
        }
        // sll control
        else if (funct == "000000")
        {
            // custom control
            return "1111";
        }
        // srl control
        else if (funct == "000010")
        {
            // custom control
            return "1001";
        }
    }
    // lw, sw, and addi control
    else if (aluOp == "00")
    {
        return "0010";
    }
    // subi control
    else if (aluOp == "01")
    {
        return "0110";
    }
    // made up for andi and ori
    else if (aluOp == "11")
    {

        // andi
        if (opcode == "001100")
            return "0000";
        // ori
        if (opcode == "001101")
            return "0001";
    }

    // incase breaks
    return "0000";
}
