/*---------------------------------------------------------------------------/
/File Name: project3Simulator.cpp
/Author:    Muhammed Muiktar
/Date:      5/12/2021
/Section:   02
/Email:     mmuktar1@umbc.edu
/
/Description: This program simulates MIPS pipline instructions
/             (multi stage EX stage)
/
/---------------------------------------------------------------------------*/

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <tuple>
#include <iomanip>
#include <algorithm>

using namespace std;

//Vector used to store opcodes and functions for instructions
typedef vector<tuple<string, string, string>> R_instructions;
R_instructions R_INSTRUCTION_LIST;

typedef vector<tuple<string, string>> I_instructions;
I_instructions I_INSTRUCTION_LIST;

//Vector represents register names with their index
typedef vector<tuple<string, int>> registerIndex;
registerIndex LIST_REG_INDEX;

typedef vector<tuple<string, int>> labelIndex;
labelIndex INST_LABEL_INDEX;

typedef vector<tuple<string, int>> label;
label instructionLabelIndex;

//Vector to store data memory and instruction memory
vector<string> list_0x00;
vector<string> list_0x100;

//vector to track stage clocks
vector<int> trackIFClock;
vector<int> trackIDClock;
vector<int> trackEXClock;
vector<int> trackMEMClock;
vector<int> trackWBClock;

//tracks index for clock tracking
int trackIndex = 0;

//registers stored in array
int registers[32];

//index indicating index of register
const int INDEX_V = 2, INDEX_A = 4, INDEX_T0 = 8, INDEX_S = 16
                    , INDEX_T8 = 24, INDEX_K = 26;

//represents pipline stages with numbers
const int FETCH = 1, DECODE = 2, EXECUTE_1 = 3, EXECUTE_2 = 4, EXECUTE_3 = 5
        , ACCESS = 6, WRITE_BACK = 7;

int PC = 0;
bool pcSrc = 0;
bool ifFlush = 0;
int clockCycle = 1;

string instructionFile;
string outputFile;
ofstream outInst;

//variables to hold varaiables for write output
bool writtenHeader = 0; 
bool firstFlush = 0;
bool finishedInst = 0;
int holdAddress = 0;
int holdPrevPC = 0;
int prevPC = 0;

//vector to queue stages for pipline
typedef vector<tuple<int, int>> Queue;
Queue queueInstructions;

//Precondition:     N/A
//Post condition:   Parses instruction file inserting into instruction memory  
void loadInstruction(string filename);

//Precondition:     N/A
//Post condition:   Stores data file into data memory
void loadData(string filename);

//Precondition:     N/A
//Post condition:   Creates file with instructions and clock cycle at each stage
void writeOutput(string filename);

//Precondition:     Instruction vectors are initalized
//Post condition:   Instruction vectors are populated with opcodes and funct for
//                  the instructions
void loadInstrucionList();

//Precondition:     Register list is initalized
//Post condition:   Popluates registers 
void loadRegister();

//Precondition:     Register list is initalized and popluated from instruction
//Post condition:   Creates a txt with register values
void writeRegisterValue();

//Precondition:     Data list is initalized and updated from instruction
//Post condition:   Creates a txt with updated data
void writeDataValue();

//-----------------------------------------------------------------------------

//Precondition:     LIST_REG_INDEX is poplutated   
//Post condition:   Returns the index of a given register as it correlates with
//                  the register's index in the register array
int getRegIndex(string reg);

//Precondition:     N/A    
//Post condition:   Returns binary number with the given number of bits
string decimalToBinaryStr (int number, int numBits); 

//-----------------------------------------------------------------------------

//Precondition:     N/A    
//Post condition:   Instructions are ran in pipline order
void runInstructions();

//Precondition:     Instrution memory is populated    
//Post condition:   Fetches instructions and populates registers
void instructionFetch();

//Precondition:     Instruction was fetched   
//Post condition:   Decodes instructions and populates registers
void instructionDecode();

//Precondition:     Instruction was decoded   
//Post condition:   Executes instructions and populates registers
void execute_iu1();

//Precondition:     Instruction was decoded   
//Post condition:   Executes instructions and populates registers
void execute_iu2();

//Precondition:     Instruction was decoded   
//Post condition:   Executes instructions and populates registers
void execute_iu3();

//Precondition:     Instrucion was executed    
//Post condition:   Access memory for intruction and populates registers
void accessMemory();

//Precondition:     Instrucion memory accessed    
//Post condition:   Writes to register in instructions and populates registers
void writeBack();

//-----------------------------------------------------------------------------

//Precondition:     N/A    
//Post condition:   Poplates id_ex registers with proper alu controls
void makeControl(string opcode, string funct);

//Precondition:     N/A    
//Post condition:   Returns operations with given variables
string ALU(string var1, string var2, string control);

//Precondition:     N/A    
//Post condition:   Returns alu control given op code and funct
string ALUcontrol(string aluOp, string funct, string opcode);

//Precondition:     N/A    
//Post condition:   Popluates instruction queue with respect to current clock cycle
void insertQueue();

//-----------------------------------------------------------------------------

//Precondition:     N/A    
//Post condition:   Detects hazards and forwards when possible
void forwarding();

//Precondition:     N/A    
//Post condition:   Detects load-use hazards then stalls accordingly
void hazardDetection();


//IF_ID Register
struct IF_ID{

    string fetchedInstruction;
    bool stopInstrucions = false;

    int nextAddress;
    bool writeFlushed = 0;

} if_idReg;

//ID_IU1 Register
struct ID_IU1{

    string readData1, readData2;
    string instrutction15_0;
    string instrutction20_16;
    string instrutction15_11;
    string opcode;

    int nextAddress;
    int jumpAddress;

    //Controls
    bool aluSrc;
    string aluOp;
    bool regDst;

    bool memWrite;
    bool memRead;

    bool regWrite;
    bool memToReg;

    bool branchBEQ = 0;
    bool branchBNE = 0;
    bool jump = 0;

    //controls for forwarding
    string registerRs;
    string registerRt;
    string registerRd;

    //tracks nops
    bool idNOP = 0, exNOP = 0, memNOP = 0, wbNOP = 0;

    //tracks branching and jumping
    bool branch_jump = 0;

    //tracks flushing
    bool writeFlushed = 0;

    //indicates IU execution
    string execute_inst;
    bool executed = 0;

} id_iu1Reg;

//IU1_IU2 Register
struct IU1_IU2{

    //stores IU1 result values
    string aluResult;
    string regDestination;

    string readData1, readData2;
    string instrutction15_0;
    string instrutction20_16;
    string instrutction15_11;
    string opcode;

    int nextAddress;
    int jumpAddress;

    //Controls
    bool aluSrc;
    string aluOp;
    bool regDst;

    bool memWrite;
    bool memRead;

    bool regWrite;
    bool memToReg;

    //might remove
    bool branchBEQ = 0;
    bool branchBNE = 0;
    bool jump = 0;

    //controls for forwarding
    string registerRs;
    string registerRt;
    string registerRd;

    //tracks nops
    bool exNOP = 0, memNOP = 0, wbNOP = 0;

    //tracks branching and jumping
    bool branch_jump = 0;

    //tracks flushing
    bool writeFlushed = 0;

    //indicates IU execution
    string execute_inst;
    bool executed = 0;

} iu1_iu2Reg;


//IU2_IU3 Register
struct IU2_IU3{

    //stores IU2 result values
    string aluResult;
    string regDestination;

    string readData1, readData2;
    string instrutction15_0;
    string instrutction20_16;
    string instrutction15_11;
    string opcode;

    int nextAddress;
    int jumpAddress;

    //Controls
    bool aluSrc;
    string aluOp;
    bool regDst;

    bool memWrite;
    bool memRead;

    bool regWrite;
    bool memToReg;

    //might remove
    bool branchBEQ = 0;
    bool branchBNE = 0;
    bool jump = 0;

    //controls for forwarding
    string registerRs;
    string registerRt;
    string registerRd;

    //tracks nops
    bool exNOP = 0, memNOP = 0, wbNOP = 0;

    //tracks branching and jumping
    bool branch_jump = 0;

    //tracks flushing
    bool writeFlushed = 0;

    //indicates IU execution
    string execute_inst;
    bool executed = 0;

} iu2_iu3Reg;

//EXE_MEM Register
struct IU3_MEM{

    string aluResult;
    string readData2;
    string regDestination;

    //Controls
    bool memWrite;
    bool memRead;

    bool regWrite;
    bool memToReg;
    
    //tracks nops
    bool memNOP = 0, wbNOP = 0;

    //tracks branching and jumping
    bool branch_jump = 0;

    //tracks flushing
    bool writeFlushed = 0;

    bool executed = 0;

} iu3_memReg;

//MEM_WB Registers
struct MEM_WB{

    string readDataMem;
    string aluResult;
    string regDestination;

    string muxALU;
    
    //Controls
    bool regWrite;
    bool memToReg;

    //tracks nops
    bool wbNOP = 0;
    
    //tracks branching and jumping
    bool branch_jump = 0;

    //tracks flushing
    bool writeFlushed = 0;
    
} mem_wbReg;

//forwarding unit
struct FORWARD_UNIT{

    //units for forwarding
    bool mem_wbRegWrite = 0;
    bool iu3_memRegWrite = 0;
    bool iu2_iu3RegWrite = 0;
    bool iu1_iu2RegWrite = 0;

    string mem_wbRegisterRd = "";
    string iu3_memRegisterRd = "";
    string iu2_iu3RegisterRd = "";
    string iu1_iu2RegisterRd = "";
    
    string registerRs = "";
    string registerRt = "";

    string forwardA = "";
    string forwardB = "";
    
    //tracks where to forward
    bool forwardID = 0;

} fw_unit;


struct HAZARD_UNIT{

    //units for hazard detection
    bool stall = 0;
    bool PCWrite = 1;
    bool id_iu1MemRead = 0;
    bool iu1_iu2MemRead = 0;
    bool iu2_iu3MemRead = 0;

    string id_iu1RegisterRt = "";
    string iu1_iu2RegisterRt = "";
    string iu2_iu3RegisterRt = "";
    
    string if_idRegisterRs = "";
    string if_idRegisterRt = "";

    string iu1_iu2RegisterRd = "";
    string iu2_iu3RegisterRd = "";

    string exe_instruction = "";
    string iu1_iu2_instruction = "";
    string iu2_iu3_instruction = "";

    bool iu1_iu2RegWrite;
    bool iu2_iu3RegWrite; 
    
    bool iu1executed = 0;
    bool iu2executed = 0;
    bool iu3executed = 0;

} hazard_unit;


int main(int argc, char *argv[]){

    try {
        
        //Number of arguments are not correct, needs four to execute program
        if (argc != 4) {

            throw "Format must be simulatorV3 <instructions> <data> <output>";
                
        }

        //Loading instructions and registers
        loadInstrucionList();
        loadRegister();

        //Parsing instructions
        loadInstruction(argv[1]);

        //Populating data memory
        loadData(argv[2]);

        outputFile = argv[3];
        instructionFile = argv[1];

        outInst.open(outputFile);

        //Runs Pipline
        runInstructions();
        finishedInst = 1; 

        //project output is written to files
        writeOutput(instructionFile);  

        outInst.close();
        writeRegisterValue();
        writeDataValue();

    } catch (const char* error){

        cout << "Error: "<< error << endl;
        cout << "Example: inst.txt data.txt output.txt" << endl;

    }

}

void loadInstrucionList(){

    //Insert R type instructions, instruction - opcode - funct
    R_INSTRUCTION_LIST.push_back(tuple<string,string,string> ("ADD", "000000", "100000"));
    R_INSTRUCTION_LIST.push_back(tuple<string,string,string> ("SUB", "000000", "100010"));
    R_INSTRUCTION_LIST.push_back(tuple<string,string,string> ("AND", "000000", "100100"));
    R_INSTRUCTION_LIST.push_back(tuple<string,string,string> ("OR", "000000", "100101"));
    R_INSTRUCTION_LIST.push_back(tuple<string,string,string> ("SLL", "000000", "000000"));
    R_INSTRUCTION_LIST.push_back(tuple<string,string,string> ("SRL", "000000", "000010"));
    R_INSTRUCTION_LIST.push_back(tuple<string,string,string> ("MULT", "000000", "011000"));

    //Insert I type instructions, instruction - opcode
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("LW", "100011"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("SW", "101011"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("LI", "001011")); 
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("ADDI", "001000"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("SUBI", "100001")); 
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("ANDI", "001100"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("ORI", "001101"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("BEQ", "000100"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("BNE", "000101"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("SLLI", "010010")); 
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("SRLI", "000011"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("MULTI", "001001"));

}

void loadRegister(){

    //Inserts needed register names to represent index
    LIST_REG_INDEX.push_back(tuple<string,int> ("$zero", 0));
    registers[0] = 0;

    LIST_REG_INDEX.push_back(tuple<string,int> ("$at", 1));
    LIST_REG_INDEX.push_back(tuple<string,int> ("$gp", 28));
    LIST_REG_INDEX.push_back(tuple<string,int> ("$sp", 29));
    LIST_REG_INDEX.push_back(tuple<string,int> ("$fp", 30));
    LIST_REG_INDEX.push_back(tuple<string,int> ("$ra", 31));

}

int getRegIndex(string reg){

    for (vector<tuple<string,int>>::iterator i = LIST_REG_INDEX.begin()
                                    ; i != LIST_REG_INDEX.end(); ++i) {

        //if register is found
        if (reg == get<0>(*i)){

            //return index
            return get<1>(*i);

       }
    }

    //gets the number from register's thrid index
    //example $s8, gets 8 and converts to int
    int addIndex = (int)reg[2] - 48;

    int index = 0;

    //finding index that represents the register integer
    if (reg[1] == 'v') {

        index = INDEX_V;

    }
    else if (reg[1] == 'a') {

        index = INDEX_A;

    }
    else if (reg[1] == 't') {

        //checks for temporary's integer
        if (addIndex >= 8) {
            addIndex -= 8;
            index = INDEX_T8;
        }
        else
            index = INDEX_T0;

    }
    else if (reg[1] == 's'){

        index = INDEX_S;

    }
    else if (reg[1] == 'k'){

        index = INDEX_K;

    }
    
    //returns computed index
    return (index + addIndex);

}

string decimalToBinaryStr (int number, int numBits){

    string makeBinary = "";
    int bit = 0;

    //while number can be divided by 2
    while (number > 0){

        bit = number % 2;
        number /= 2;

        //makes binary
        makeBinary.insert(0, to_string(bit));

    }

    //makes sure binary has the corrct number of bits
    if (int(makeBinary.size()) > numBits) {            

        makeBinary.resize(numBits);
    }
    else if (int(makeBinary.size()) < numBits) {

        while (int(makeBinary.size()) != numBits)
            makeBinary.insert(0, "0");

    }

    return makeBinary;
}

void loadInstruction(string filename){

    ifstream loadInst;
    loadInst.open(filename);

    //file does not exist
    if (loadInst.fail()) {

        cerr << "Invalid instructions file " << filename << endl;
        exit(0);
    }

    string getInst;

    int size = 0;

    //goes over instructions for labels indexes
    while(loadInst >> getInst) {

        if (getInst[getInst.size() - 1] == 58) {

            //gets label
            getInst.resize(getInst.size() - 1);
            INST_LABEL_INDEX.push_back(tuple<string,int>(getInst, size));

        }

        string tempInst;

        //converts getInst to uppercase
        for (int i = 0; i < int(getInst.length()); i ++){

            tempInst.push_back(toupper(getInst[i]));

        }

        getInst = tempInst;
        tempInst = "";

        //checking for number of instructions
        for (vector<tuple<string,string,string>>::iterator i = R_INSTRUCTION_LIST.begin()
                                                    ; i != R_INSTRUCTION_LIST.end(); ++i){
            if (getInst == get<0>(*i)) {
                size++;
            }

        }

        for (vector<tuple<string,string>>::iterator i = I_INSTRUCTION_LIST.begin()
                                            ; i != I_INSTRUCTION_LIST.end(); ++i){
            
            if (getInst == get<0>(*i)) {
                size++;
            }
        }

        if (getInst == "J" || getInst == "HLT" || getInst =="NOP")
            size++;
        
    }

    //resetting ifstream
    loadInst.close();
    loadInst.open(filename);

    string getReg1, getReg2, getReg3, getCons;
    string shamt;

    string temp, tempInst;
    string tempBinary = "";

    //while there is something to read
    while (loadInst >> getInst) {
        
        //checks for labels
        while (getInst[getInst.size() - 1] == 58) {

            //skips over labels
            loadInst >> getInst;

        }

        //converts getInst to uppercase
        for (int i = 0; i < int(getInst.length()); i ++){

            tempInst.push_back(toupper(getInst[i]));

        }

        getInst = tempInst;
        tempInst = "";
        
        //checks instruction in R_INSTRUCTIONS
        for (vector<tuple<string,string,string>>::iterator i = R_INSTRUCTION_LIST.begin()
                                                    ; i != R_INSTRUCTION_LIST.end(); ++i){
            
            if (getInst == get<0>(*i)) {
                
                //adds opcode
                tempBinary.append(get<1>(*i));
                
                //gets register1
                getline(loadInst, getReg1, ',');
                
                //remove white space
                getReg1.erase(remove_if(getReg1.begin(), getReg1.end(), ::isspace), getReg1.end());

                //gets register2
                getline(loadInst, getReg2, ',');

                //remove white space
                getReg2.erase(remove_if(getReg2.begin(), getReg2.end(), ::isspace), getReg2.end());

                //gets register3
                loadInst >> getReg3;

                //adds rs rt and rd
                tempBinary.append(decimalToBinaryStr(getRegIndex(getReg2), 5));
                tempBinary.append(decimalToBinaryStr(getRegIndex(getReg3), 5));
                tempBinary.append(decimalToBinaryStr(getRegIndex(getReg1), 5));

                shamt = "00000";
                
                //adds shamt and funct
                tempBinary.append(shamt);
                tempBinary.append(get<2>(*i));

            }
        }

        //checks instruction in I_INSTRUCTIONS
        for (vector<tuple<string,string>>::iterator i = I_INSTRUCTION_LIST.begin()
                                            ; i != I_INSTRUCTION_LIST.end(); ++i){
            
            if (getInst == get<0>(*i)) {
                
                //adds opcode
                tempBinary.append(get<1>(*i));
                
                //gets register1
                getline(loadInst, getReg1, ',');

                //remove white space
                getReg1.erase(remove_if(getReg1.begin(), getReg1.end(), ::isspace), getReg1.end());
                
                //if instruction is li
                if (getInst == "LI") {

                    getReg2 = "00000";

                }
                //if instructions is lw or sw
                else if (getInst == "LW" || getInst == "SW") {
                    
                    loadInst >> temp;

                    int first_paren = temp.find("(");
                    int second_paren = temp.find(")");
                    
                    //gets register2 for lw or sw
                    getReg2 = temp.substr(first_paren + 1, (second_paren - (first_paren + 1)));
                    
                    //remove white space
                    getReg2.erase(remove_if(getReg2.begin(), getReg2.end(), ::isspace), getReg2.end());
                    

                }
                else {
                    
                    //gets register2
                    getline(loadInst, getReg2, ',');

                    //remove white space
                    getReg2.erase(remove_if(getReg2.begin(), getReg2.end(), ::isspace), getReg2.end());
                    
                }

                if (getInst == "LW" || getInst == "SW") {
                    //gets constant for lw or sw 
                    getCons = temp.substr(0, temp.find("("));

                    //remove white space
                    getCons.erase(remove_if(getCons.begin(), getCons.end(), ::isspace), getCons.end());
                }
                else {

                    //gets constant/address
                    loadInst >> getCons;
                    
                }

                //adds rs rt
                if (getInst == "BEQ" || getInst == "BNE") {
                    
                    tempBinary.append(decimalToBinaryStr(getRegIndex(getReg1), 5));
                    tempBinary.append(decimalToBinaryStr(getRegIndex(getReg2), 5));

                }
                else {
                    
                    tempBinary.append(decimalToBinaryStr(getRegIndex(getReg2), 5));
                    tempBinary.append(decimalToBinaryStr(getRegIndex(getReg1), 5));
                
                }

                //check if hex
                if (getCons[getCons.size() - 1] == 'h') {

                    //converts to hex    
                    getCons.replace(getCons.size() - 1, 1 , "");
                    getCons.insert(0, "0x");
                    
                    //adds constant
                    tempBinary.append(decimalToBinaryStr(stoi(getCons, nullptr, 16), 16));

                } 
                else {
                    
                    if (getInst == "BEQ" || getInst == "BNE") {
                        
                        //checks for label index
                        for (int i = 0; i < int(INST_LABEL_INDEX.size()); i++) {
                            
                            if (get<0>(INST_LABEL_INDEX[i]) == getCons) 
                                //adds label index
                                tempBinary.append(decimalToBinaryStr(get<1>(INST_LABEL_INDEX[i]), 16));
                                
                        }
                    }
                    else {

                        string makeNegative;
                        //adds constant
                        if (stoi(getCons) < 0) {

                            getCons.replace(0, 1, "");
                            makeNegative = "1";
                        }
                        else {

                            makeNegative = "0";

                        }

                        makeNegative.append(decimalToBinaryStr(stoi(getCons), 15));
                        tempBinary.append(makeNegative);
                    
                    }
                    
                }
            }
        }

        //checks for jump instruction
        if (getInst == "J") {

            //adds opcode
            tempBinary.append("000010");
            loadInst >> getCons;

            //looks for label in list
            for (int i = 0; i < int(INST_LABEL_INDEX.size()); i++) {
                            
                if (get<0>(INST_LABEL_INDEX[i]) == getCons) {
                                
                    //gets label index
                    getCons = decimalToBinaryStr(get<1>(INST_LABEL_INDEX[i]), 26);
                                
                }
            }
            tempBinary.append(getCons);
        }

        //if instruction is to halt
        if (getInst == "HLT") {

            list_0x00.push_back("11111100000000000000000000000000");

        }
        //checks a nop
        else if (getInst == "NOP") {

            //reads $zero
            loadInst >> temp;
            loadInst >> temp;
            loadInst >> temp;
            list_0x00.push_back("00000000000000000000000000000000");

        }
        else {
            list_0x00.push_back(tempBinary);
        }
        
        tempBinary = "";
    }

    loadInst.close();
}

void loadData(string filename){

    ifstream loadData;
    loadData.open(filename);

    //if file does not exist
    if (loadData.fail()) {

        cerr << "Invalid data file " << filename << endl;
        exit(0);
    }

    string getData;

    //while not at end of file
    while (loadData >> getData){
        
        //adds data to data memory
        list_0x100.push_back(getData);

    }

    loadData.close();
}

void writeOutput(string filename) {

    ifstream loadInst;
    loadInst.open(filename);

    //file does not exist
    if (loadInst.fail()) {

        cerr << "Invalid instructions file " << filename << endl;
        exit(0);
    }

    string getFirst, getInst, temp;
    int index = 0;
    
    int tempAddress = 0;

    //header
    if (finishedInst && !writtenHeader){
        
        outInst.width(66);
        outInst << right << "IF   ID   EX   MEM  WB";
        outInst << "\n";


    }
    //first flushing is written to output file
    else if (!firstFlush) {

        outInst.width(66);
        outInst << right << "IF   ID   EX   MEM  WB";
        outInst << "\n";
        writtenHeader = 1;

        tempAddress = 0;
    }

    //flushing when first flush has been written
    if (mem_wbReg.writeFlushed && firstFlush) {

        tempAddress = holdAddress; 
         
    }
    //finished instruction
    else if (finishedInst){

        tempAddress = holdAddress; 
        prevPC++;
        holdPrevPC = prevPC;
        
    }   

    //while there are instructions to read
    while (loadInst >> getFirst) {
        
        //writting within the proper indexes
        if (index >= tempAddress && index <= holdPrevPC) {    
            
            //at label
            if (getFirst[getFirst.size() - 1] == 58) {
                
                //insert label 
                outInst.width(10); 
                outInst << left << getFirst;
                
                loadInst >> getFirst;

                while (getFirst[getFirst.size() - 1] == 58) {

                    outInst << "\n";

                    //insert next label
                    outInst.width(10); 
                    outInst << left << getFirst;

                    loadInst >> getFirst;

                }

                //inserts instructions
                outInst.width(10);
                outInst << left << getFirst;

            }
            else {
        
                //insert instruction
                outInst.width(10);
                outInst << left << "";
                outInst.width(10);
                outInst << left << getFirst;

            }

            temp = "";
            //converts to uppercase
            for (int i = 0; i < int(getFirst.length()); i ++){

                temp.push_back(toupper(getFirst[i]));

            }

            getFirst = temp;
            temp = "";

            //checks end instruction call
            if (getFirst == "HLT") {
                
                //inserts spacing
                outInst.width(24);
                outInst << left << "";
            
            } 
            else {

                //load register1
                getline(loadInst, getInst, ',');

                //remove white space
                getInst.erase(remove_if(getInst.begin(), getInst.end(), ::isspace), getInst.end());
            
                outInst.width(7);
                getInst.append(",");
                outInst << left << getInst;

            
                //checks specific instruction call
                if (getFirst == "LW" || getFirst == "SW" || getFirst == "LI" || getFirst == "J") {
                    
                    //gets next part of instruction
                    loadInst >> getInst;

                    outInst.width(17);

                    //jump instruction
                    if (getFirst == "J")
                        outInst << left << "";
                    else
                        outInst << left << getInst;

                }
                else {

                    getline(loadInst, getInst, ',');

                    //remove white space
                    getInst.erase(remove_if(getInst.begin(), getInst.end(), ::isspace), getInst.end());

                    //inserts next two instrucions
                    outInst.width(7);
                    getInst.append(",");
                    outInst << left << getInst;

                    loadInst >> getInst;
        
                    outInst.width(10); 
                    outInst << left << getInst;

                }
                
            }

            //inserts clock cycle from each stage
            if (trackIndex < int(trackIFClock.size())) {

                outInst.width(5);

                //if tracking during nop
                if (trackIFClock.at(trackIndex) == 0)
                    outInst << left << "";
                else
                    outInst << left << trackIFClock.at(trackIndex);

            }
            if (trackIndex < int(trackIDClock.size())) {

                outInst.width(5);

                //if tracking during nop
                if (trackIDClock.at(trackIndex) == 0)
                    outInst << left << "";
                else
                    outInst << left << trackIDClock.at(trackIndex);

            }
            if (trackIndex < int(trackEXClock.size())) {

                outInst.width(5);

                //if tracking during nop
                if (trackEXClock.at(trackIndex) == 0)
                    outInst << left << "";
                else
                    outInst << left << trackEXClock.at(trackIndex);

            }
            if (trackIndex < int(trackMEMClock.size())) {
                
                outInst.width(5);

                //if tracking during nop
                if (trackMEMClock.at(trackIndex) == 0)
                    outInst << left << "";
                else
                    outInst << left << trackMEMClock.at(trackIndex);

            }
            if (trackIndex < int(trackWBClock.size())) {
                
                outInst.width(5);

                //if tracking during nop
                if (trackWBClock.at(trackIndex) == 0)
                    outInst << left << "";
                else
                    outInst << left << trackWBClock.at(trackIndex);

            }
           
            trackIndex++;
            outInst << "\n";
        
        }
        else {
            
            //gets rest of instructions
            if (getFirst[getFirst.size() - 1] != 58)
                getline(loadInst, temp);

            temp = "";
            
        }
        index++;
        
    }  

    //indicates the first flush
    if (mem_wbReg.writeFlushed)
        firstFlush = 1;

    loadInst.close();
}

void writeRegisterValue(){

    ofstream outReg("register.txt");

    string registerValue = "";
    int num;

    //writes register values
    for (int i = 0; i < 32; i++) {

        num = registers[i];

        if (registers[i] < 0) {

            registerValue = "-";
            num *= -1;

        }
        else {

            registerValue = "";

        }

        registerValue.append(decimalToBinaryStr(num, 32));

        outReg << "%"<< i << " = " 
        << registerValue << endl;

    }

}

void writeDataValue(){

    ofstream outData("dataUpdate.txt");

    //writes data memory
    for (int i = 0; i <  int(list_0x100.size()); i++) {

        outData << list_0x100.at(i) << endl;

    }
}

void insertQueue(){

    //inserts functions queue with respect to clock cycle
    queueInstructions.push_back(tuple<int,int> (FETCH, clockCycle));
    queueInstructions.push_back(tuple<int,int> (DECODE, clockCycle + 1));
    queueInstructions.push_back(tuple<int,int> (EXECUTE_1, clockCycle + 2));
    queueInstructions.push_back(tuple<int,int> (EXECUTE_2, clockCycle + 3));
    queueInstructions.push_back(tuple<int,int> (EXECUTE_3, clockCycle + 4));
    queueInstructions.push_back(tuple<int,int> (ACCESS, clockCycle + 5));
    queueInstructions.push_back(tuple<int,int> (WRITE_BACK, clockCycle + 6));

}

void runInstructions() {

    //starts queue
    insertQueue();

    //while instruct is not halted
    while (!if_idReg.stopInstrucions) {

        //loops through queue
        for (vector<tuple<int,int>>::iterator i = queueInstructions.begin()
                                    ; i != queueInstructions.end(); ++i) {

            //checks if stage needs to be executed given clock cycle
            if (get<1>(*i) == clockCycle) {
                
                //finds stage to run
                if (get<0>(*i) == FETCH) {
                    instructionFetch();
                }
                else if (get<0>(*i) == DECODE){
                    instructionDecode();
                }
                else if (get<0>(*i) == EXECUTE_1){
                    execute_iu1();       
                }
                else if (get<0>(*i) == EXECUTE_2){
                    execute_iu2();       
                }
                else if (get<0>(*i) == EXECUTE_3){
                    execute_iu3();       
                }
                else if (get<0>(*i) == ACCESS){
                    accessMemory();    
                }
                else if (get<0>(*i) == WRITE_BACK){
                    writeBack();
                }
                
                //removes stage
                queueInstructions.erase(i);
                i--;
                
            }
        }

        clockCycle++;

        //stops queue 
        if (!if_idReg.stopInstrucions)
            insertQueue();

    }
}

void instructionFetch(){

    //stop fetching when HLT is called in decode
    if (!if_idReg.stopInstrucions) {

        if (hazard_unit.PCWrite) {
            
            //gets instruction memory
            if (id_iu1Reg.jump) {
                
                prevPC = PC;
                PC = id_iu1Reg.jumpAddress;
                if_idReg.fetchedInstruction = list_0x00[PC];
               
            }
            else if (pcSrc) {

                prevPC = PC;
                PC = id_iu1Reg.nextAddress;
                if_idReg.fetchedInstruction = list_0x00[PC];

            }
            else {
                
                prevPC = PC;
                if_idReg.fetchedInstruction = list_0x00[PC];
                PC++;

            }

            //passes address
            if_idReg.nextAddress = PC;
        }

    }


    //flush detected
    if (ifFlush) {
        
        if_idReg.writeFlushed = 1;

        holdAddress = if_idReg.nextAddress;
        holdPrevPC = prevPC;
        
        //flushes with a nop instruction
        if_idReg.fetchedInstruction = "00000000000000000000000000000000";
        
        //nops
        id_iu1Reg.idNOP = 1;
        id_iu1Reg.exNOP = 1;
        id_iu1Reg.memNOP = 1;
        id_iu1Reg.wbNOP = 1;

    }
    else {

        if_idReg.writeFlushed = 0;
    
    }

    //hazard not detected
    if (!hazard_unit.stall)
        trackIFClock.push_back(clockCycle);
    
}

void instructionDecode(){

    //updates controls id_iu1Reg
    string opcode = if_idReg.fetchedInstruction.substr(0,6);

    //checks if halt
    if (opcode == "111111") {

        if_idReg.stopInstrucions = true;

    }
    else {

        string getFunct = if_idReg.fetchedInstruction.substr(26,6);

        //gets rs 
        string register1 = if_idReg.fetchedInstruction.substr(6,5); 
        string readData1 = "0";

        int checkValue1 = registers[stoi(register1,nullptr,2)];

        if (checkValue1 < 0){

            checkValue1 *= -1;
            readData1 = "1";

        }

        readData1.append(decimalToBinaryStr(checkValue1, 31));
        id_iu1Reg.readData1 = readData1;

        //gets rt
        string register2 = if_idReg.fetchedInstruction.substr(11,5);
        string readData2 = "0";
        
        int checkValue2 = registers[stoi(register2,nullptr,2)];

        if (checkValue2 < 0){

            checkValue2 *= -1;
            readData2 = "1";

        }

        readData2.append(decimalToBinaryStr(checkValue2, 31));
        id_iu1Reg.readData2 = readData2;

        //gets rd in R format
        id_iu1Reg.instrutction15_11 = if_idReg.fetchedInstruction.substr(16,5);

        //rd in I format
        id_iu1Reg.instrutction20_16 = if_idReg.fetchedInstruction.substr(11,5);

        //hazard checking
        hazard_unit.if_idRegisterRs = register1;
        hazard_unit.if_idRegisterRt = register2;
    
        hazardDetection();

        //hazarded detected
        if (hazard_unit.stall) {

            //sets controls to 0
            id_iu1Reg.regDst = 0;
            id_iu1Reg.aluSrc = 0;
            id_iu1Reg.memToReg = 0;
            id_iu1Reg.regWrite = 0;
            id_iu1Reg.memRead = 0;
            id_iu1Reg.memWrite = 0;
            id_iu1Reg.branchBEQ = 0;
            id_iu1Reg.branchBNE = 0;
            id_iu1Reg.jump = 0;

            id_iu1Reg.aluOp = "000";

            id_iu1Reg.idNOP = 1;
            id_iu1Reg.exNOP = 1;
            id_iu1Reg.memNOP = 1;
            id_iu1Reg.wbNOP = 1;

        } else {

            //populates control values in id_iu1 register
            makeControl(opcode, getFunct);

            //not flushing
            if (!ifFlush) {
                id_iu1Reg.idNOP = 0;
                id_iu1Reg.exNOP = 0;
                id_iu1Reg.memNOP = 0;
                id_iu1Reg.wbNOP = 0;
            }
            

        }

        //forwards values to ID stage
        fw_unit.registerRs = register1;
        fw_unit.registerRt = register2;
        fw_unit.forwardID = 1;
        
        forwarding();
        
        if (fw_unit.forwardA == "010"){
            readData1 = iu2_iu3Reg.aluResult;
        }
        else if (fw_unit.forwardA == "011"){
            readData1 = iu1_iu2Reg.aluResult;
        }
        else if (fw_unit.forwardA == "100"){
            readData1 = iu3_memReg.aluResult;
        }
        else if (fw_unit.forwardA == "001"){
            readData1 = mem_wbReg.muxALU;  
        }
        

        if (fw_unit.forwardB == "010"){
            readData2 = iu2_iu3Reg.aluResult;
        }
        else if (fw_unit.forwardB == "011"){
            readData2 = iu1_iu2Reg.aluResult;
        }
        else if (fw_unit.forwardB == "100"){
            readData2 = iu3_memReg.aluResult;
        }
        else if (fw_unit.forwardB == "001"){
            readData2 = mem_wbReg.muxALU;  
        }
        

        string tempString = if_idReg.fetchedInstruction.substr(16,16);
        int tempConst;

        //if lw or sw
        if (opcode == "100011" || opcode == "101011") {

            int negativeValue = 1;

            if (tempString[0] == '1') {

                tempString.replace(0, 1, "");
                negativeValue = -1;

            }

            //makes up word difference in address
            tempConst = stoi(tempString,nullptr,2) * negativeValue;
            tempConst /= 4;
            id_iu1Reg.instrutction15_0 = decimalToBinaryStr(tempConst, 16);

        } 
        else {
            
            //gets last 16 bits for constant/address
            id_iu1Reg.instrutction15_0 = if_idReg.fetchedInstruction.substr(16,16);

        }

        //shifts constant/address to 32 bits
        while (int(id_iu1Reg.instrutction15_0.size()) != 32)
                id_iu1Reg.instrutction15_0.insert(1, "0");

        //calucates xor
        int checkZero = stoi(readData1, nullptr, 2) ^ stoi(readData2, nullptr, 2);
        
        bool zero;
        //checks if result is zero
        if (checkZero == 0)
            zero = 1;
        else
            zero = 0;
        
        //checks for branch or jumping flush
        if ((id_iu1Reg.branchBEQ && zero) || 
            (id_iu1Reg.branchBNE && !zero) || id_iu1Reg.jump) {
            
            //if jump instruction
            if (id_iu1Reg.jump)
                pcSrc = 0;
            else
                pcSrc = 1;

            //flush
            ifFlush = 1;

            //nops
            id_iu1Reg.exNOP = 1;
            id_iu1Reg.memNOP = 1;
            id_iu1Reg.wbNOP = 1;
        
        }
        //checks for branching without flushing
        else if (id_iu1Reg.branchBEQ || id_iu1Reg.branchBNE) {
            
            pcSrc = 0;
            ifFlush = 0;

            //nops
            id_iu1Reg.exNOP = 1;
            id_iu1Reg.memNOP = 1;
            id_iu1Reg.wbNOP = 1;

        }
        //normal execution
        else {

            pcSrc = 0;
            ifFlush = 0;

        }

        //checks for branching or jumping (to track clock)
        if (id_iu1Reg.branchBEQ || id_iu1Reg.branchBNE || id_iu1Reg.jump)
            id_iu1Reg.branch_jump = 1;
        else
            id_iu1Reg.branch_jump = 0;

        //fixes address format
        string address = id_iu1Reg.instrutction15_0;
        address.replace(0, 1, "");

        //jump and conditional addresses
        int indexAddress = stoi(address , nullptr, 2);
        id_iu1Reg.nextAddress = indexAddress;
        id_iu1Reg.jumpAddress = stoi(if_idReg.fetchedInstruction.substr(6, 26), nullptr, 2);

        id_iu1Reg.opcode = opcode;

        id_iu1Reg.registerRs =  register1;
        id_iu1Reg.registerRt = register2;

        id_iu1Reg.executed = 0;

    }

    id_iu1Reg.writeFlushed = if_idReg.writeFlushed;

    //tracks with the right conditions
    if (!hazard_unit.stall && !id_iu1Reg.idNOP)
        trackIDClock.push_back(clockCycle);
    else if (id_iu1Reg.branch_jump || id_iu1Reg.writeFlushed)
        trackIDClock.push_back(0);
}

void execute_iu1(){

    //hazard
    hazard_unit.id_iu1MemRead = id_iu1Reg.memRead;
    hazard_unit.id_iu1RegisterRt = id_iu1Reg.registerRt;

    string registerDes;

    //gets register destination
    if (id_iu1Reg.regDst)
        registerDes = id_iu1Reg.instrutction15_11;
    else
        registerDes = id_iu1Reg.instrutction20_16;

    
    //forwarding
    fw_unit.registerRs = id_iu1Reg.registerRs;
    fw_unit.registerRt = id_iu1Reg.registerRt; 
    
    fw_unit.forwardID = 0;
    forwarding();

    string dataA, dataB;

    //mux created with forwarding
    if (fw_unit.forwardA == "010"){
        dataA = iu2_iu3Reg.aluResult;
    }
    else if (fw_unit.forwardA == "011"){
        dataA = iu1_iu2Reg.aluResult;
    }
    else if (fw_unit.forwardA == "100"){
        dataA = iu3_memReg.aluResult;
    }
    else if (fw_unit.forwardA == "001"){
        dataA = mem_wbReg.muxALU;  
    }
    else if (fw_unit.forwardA == "000"){
        dataA = id_iu1Reg.readData1;
    }

    if (fw_unit.forwardB == "010"){
        dataB = iu2_iu3Reg.aluResult;
    }
    else if (fw_unit.forwardB == "011"){
        dataB = iu1_iu2Reg.aluResult;
    }
    else if (fw_unit.forwardB == "100"){
        dataB = iu3_memReg.aluResult;
    }
    else if (fw_unit.forwardB == "001"){
        dataB = mem_wbReg.muxALU;  
    }
    else if (fw_unit.forwardB == "000"){
        dataB = id_iu1Reg.readData2;
    }

    iu1_iu2Reg.regDestination = registerDes;

    if (id_iu1Reg.execute_inst == "01") {

        string getFunct = id_iu1Reg.instrutction15_0.substr(26,6);

        //gets alu control
        string getAluControl = ALUcontrol(id_iu1Reg.aluOp, getFunct, id_iu1Reg.opcode);
        string getALUResult;
        
        //check alusrc and gets result
        if (id_iu1Reg.aluSrc) {
            getALUResult = ALU(dataA, id_iu1Reg.instrutction15_0, getAluControl);
        }
        else 
            getALUResult = ALU(dataA, dataB, getAluControl);


        iu1_iu2Reg.aluResult = getALUResult;
        iu1_iu2Reg.executed = 1;
    }
    else {

        iu1_iu2Reg.executed = id_iu1Reg.executed;

    }


    //passes values to next IU
    iu1_iu2Reg.readData1 = dataA;
    iu1_iu2Reg.readData2 = dataB;

    iu1_iu2Reg.instrutction15_0 = id_iu1Reg.instrutction15_0;
    iu1_iu2Reg.opcode = id_iu1Reg.opcode;

    iu1_iu2Reg.nextAddress = id_iu1Reg.nextAddress;
    iu1_iu2Reg.jumpAddress = id_iu1Reg.jumpAddress;

    //Controls
    iu1_iu2Reg.aluSrc = id_iu1Reg.aluSrc;
    iu1_iu2Reg.aluOp = id_iu1Reg.aluOp;

    //passes controls for rest of execution
    iu1_iu2Reg.memWrite = id_iu1Reg.memWrite;
    iu1_iu2Reg.memRead = id_iu1Reg.memRead;

    iu1_iu2Reg.regWrite = id_iu1Reg.regWrite;
    iu1_iu2Reg.memToReg = id_iu1Reg.memToReg;

    //controls for forwarding
    iu1_iu2Reg.registerRs = id_iu1Reg.registerRs;
    iu1_iu2Reg.registerRt = id_iu1Reg.registerRt;

    //tracks nops
    iu1_iu2Reg.exNOP = id_iu1Reg.exNOP;
    iu1_iu2Reg.memNOP = id_iu1Reg.memNOP;
    iu1_iu2Reg.wbNOP = id_iu1Reg.wbNOP;

    //tracks flushing
    iu1_iu2Reg.writeFlushed = id_iu1Reg.writeFlushed;

    //tracks branching and jumping
    iu1_iu2Reg.branch_jump = id_iu1Reg.branch_jump;

    //indicates IU execution
    iu1_iu2Reg.execute_inst = id_iu1Reg.execute_inst;

}

void execute_iu2(){
    
    //hazard
    hazard_unit.iu1_iu2MemRead = iu1_iu2Reg.memRead;
    hazard_unit.iu1_iu2RegisterRt = iu1_iu2Reg.registerRt;

    hazard_unit.iu1_iu2RegisterRd = iu1_iu2Reg.regDestination;
    hazard_unit.iu1_iu2RegWrite = iu1_iu2Reg.regWrite;

    //forwarding
    fw_unit.iu1_iu2RegisterRd = iu1_iu2Reg.regDestination;
    fw_unit.iu1_iu2RegWrite = iu1_iu2Reg.regWrite;

    if (iu1_iu2Reg.execute_inst == "10") {

        string getFunct = iu1_iu2Reg.instrutction15_0.substr(26,6);

        //gets alu control
        string getAluControl = ALUcontrol(iu1_iu2Reg.aluOp, getFunct, iu1_iu2Reg.opcode);
        string getALUResult;
        
        string dataA = iu1_iu2Reg.readData1;
        string dataB = iu1_iu2Reg.readData2;
        
        //check alusrc and gets result
        if (iu1_iu2Reg.aluSrc) 
            getALUResult = ALU(dataA, iu1_iu2Reg.instrutction15_0, getAluControl);

        else 
            getALUResult = ALU(dataA, dataB, getAluControl);

        
        iu2_iu3Reg.aluResult = getALUResult;
        iu2_iu3Reg.executed = 1;

    }
    else if (iu1_iu2Reg.execute_inst == "01") {

        iu2_iu3Reg.aluResult = iu1_iu2Reg.aluResult;
        iu2_iu3Reg.executed = iu1_iu2Reg.executed;
        
    }
    else {

        iu2_iu3Reg.executed = iu1_iu2Reg.executed;

    }

    //passes values to next IU
    iu2_iu3Reg.regDestination = iu1_iu2Reg.regDestination;

    iu2_iu3Reg.readData1 = iu1_iu2Reg.readData1;
    iu2_iu3Reg.readData2 = iu1_iu2Reg.readData2;

    iu2_iu3Reg.instrutction15_0 = iu1_iu2Reg.instrutction15_0;
    iu2_iu3Reg.opcode = iu1_iu2Reg.opcode;

    iu2_iu3Reg.nextAddress = iu1_iu2Reg.nextAddress;
    iu2_iu3Reg.jumpAddress = iu1_iu2Reg.jumpAddress;

    //Controls
    iu2_iu3Reg.aluSrc = iu1_iu2Reg.aluSrc;
    iu2_iu3Reg.aluOp = iu1_iu2Reg.aluOp;
        
    //passes controls for rest of execution
    iu2_iu3Reg.memWrite = iu1_iu2Reg.memWrite;
    iu2_iu3Reg.memRead = iu1_iu2Reg.memRead;

    iu2_iu3Reg.regWrite = iu1_iu2Reg.regWrite;
    iu2_iu3Reg.memToReg = iu1_iu2Reg.memToReg;

    //controls for forwarding
    iu2_iu3Reg.registerRs = iu1_iu2Reg.registerRs;
    iu2_iu3Reg.registerRt = iu1_iu2Reg.registerRt;

    //tracks nops
    iu2_iu3Reg.exNOP = iu1_iu2Reg.exNOP;
    iu2_iu3Reg.memNOP = iu1_iu2Reg.memNOP;
    iu2_iu3Reg.wbNOP = iu1_iu2Reg.wbNOP;

    //tracks flushing
    iu2_iu3Reg.writeFlushed = iu1_iu2Reg.writeFlushed;

    //tracks branching and jumping
    iu2_iu3Reg.branch_jump = iu1_iu2Reg.branch_jump;

    //indicates IU execution
    iu2_iu3Reg.execute_inst = iu1_iu2Reg.execute_inst;

}

void execute_iu3(){

    //hazard
    hazard_unit.iu2_iu3MemRead = iu2_iu3Reg.memRead;
    hazard_unit.iu2_iu3RegisterRt = iu2_iu3Reg.registerRt;

    hazard_unit.iu2_iu3RegisterRd = iu2_iu3Reg.regDestination;
    hazard_unit.iu2_iu3RegWrite = iu2_iu3Reg.regWrite;

    //forwarding
    fw_unit.iu2_iu3RegisterRd = iu2_iu3Reg.regDestination;
    fw_unit.iu2_iu3RegWrite = iu2_iu3Reg.regWrite;

    if (iu2_iu3Reg.execute_inst == "11") {

        string getFunct = iu2_iu3Reg.instrutction15_0.substr(26,6);

        //gets alu control
        string getAluControl = ALUcontrol(iu2_iu3Reg.aluOp, getFunct, iu2_iu3Reg.opcode);
        string getALUResult;
        
        string dataA = iu2_iu3Reg.readData1;
        string dataB = iu2_iu3Reg.readData2;

        //check alusrc and gets result
        if (iu2_iu3Reg.aluSrc) 
            getALUResult = ALU(dataA, iu2_iu3Reg.instrutction15_0, getAluControl);

        else 
            getALUResult = ALU(dataA, dataB, getAluControl);


        iu3_memReg.aluResult = getALUResult;

    }
    else {

        iu3_memReg.aluResult = iu2_iu3Reg.aluResult;
    
    }

    hazard_unit.iu3executed = 1;

    iu3_memReg.regDestination = iu2_iu3Reg.regDestination;
    iu3_memReg.readData2 = iu2_iu3Reg.readData2;
    
    //passes controls for rest of execution
    iu3_memReg.memWrite = iu2_iu3Reg.memWrite;
    iu3_memReg.memRead = iu2_iu3Reg.memRead;

    iu3_memReg.regWrite = iu2_iu3Reg.regWrite;
    iu3_memReg.memToReg = iu2_iu3Reg.memToReg;

    //tracks nops
    iu3_memReg.memNOP = iu2_iu3Reg.memNOP;
    iu3_memReg.wbNOP = iu2_iu3Reg.wbNOP;

    //tracks flushing
    iu3_memReg.writeFlushed = iu2_iu3Reg.writeFlushed;

    //tracks branching and jumping
    iu3_memReg.branch_jump = iu2_iu3Reg.branch_jump;

    //tracks with the right conditions
    if (!iu2_iu3Reg.exNOP)
        trackEXClock.push_back(clockCycle);
    else if (iu2_iu3Reg.branch_jump || iu2_iu3Reg.writeFlushed)
        trackEXClock.push_back(0);
}
void accessMemory(){

    string getAddress = iu3_memReg.aluResult;
    string getWriteData = iu3_memReg.readData2;

    mem_wbReg.readDataMem = "";

    //checks to write to memory
    if (iu3_memReg.memWrite) 

        //changes data, makes up for index at 0x100
        list_0x100[stoi(getAddress,nullptr,2) - 256] = getWriteData;

    //checks to read from address
    if (iu3_memReg.memRead) {

        //passes whats read to mem_wb register
        mem_wbReg.readDataMem = list_0x100[stoi(getAddress,nullptr,2) - 256];
       
    }

    //passes to mem_wb register
    mem_wbReg.aluResult = iu3_memReg.aluResult;
    mem_wbReg.regDestination = iu3_memReg.regDestination;

    mem_wbReg.regWrite = iu3_memReg.regWrite;
    mem_wbReg.memToReg = iu3_memReg.memToReg;

    //forwarding
    fw_unit.iu3_memRegisterRd = iu3_memReg.regDestination;
    fw_unit.iu3_memRegWrite = iu3_memReg.regWrite;
    
    //nops
    mem_wbReg.wbNOP = iu3_memReg.wbNOP;

    //tracks flushing
    mem_wbReg.writeFlushed = iu3_memReg.writeFlushed;

    //tracks branch and jumping
    mem_wbReg.branch_jump = iu3_memReg.branch_jump;

    //tracks with the right conditions
    if (!iu3_memReg.memNOP)
        trackMEMClock.push_back(clockCycle);
    else if (iu3_memReg.branch_jump || iu3_memReg.writeFlushed)
        trackMEMClock.push_back(0);
}
void writeBack(){

    string writeRegister = mem_wbReg.regDestination;
    string writeData;

    //forwarding
    fw_unit.mem_wbRegisterRd = mem_wbReg.regDestination;
    fw_unit.mem_wbRegWrite = mem_wbReg.regWrite;

    //checks to write register from memory
    if (mem_wbReg.memToReg) 
        writeData = mem_wbReg.readDataMem;
    else
        writeData = mem_wbReg.aluResult;
    
    
    mem_wbReg.muxALU = writeData;
    
    //checks to write to register
    if (mem_wbReg.regWrite) {
        
        int negativeValue = 1;
        if (writeData[0] == '1') {
            
            negativeValue = -1;
            writeData.replace(0, 1, "");
        
        }
    
        //cannot write to $zero
        if (stoi(writeRegister,nullptr,2) != 0)
            registers[stoi(writeRegister,nullptr,2)] = stoi(writeData,nullptr,2) * negativeValue;

    }

    //tracks with the right conditions
    if (!mem_wbReg.wbNOP)
        trackWBClock.push_back(clockCycle);
    else if (mem_wbReg.branch_jump || mem_wbReg.writeFlushed)
        trackWBClock.push_back(0);

    //writes output after stages have been tracked
    if (mem_wbReg.writeFlushed) {

        writeOutput(instructionFile);

    }
}

void makeControl(string opcode, string funct){

    //r-type opcode
    if (opcode == "000000"){

        id_iu1Reg.regDst = 1;
        id_iu1Reg.aluSrc = 0;
        id_iu1Reg.memToReg = 0;
        id_iu1Reg.regWrite = 1;
        id_iu1Reg.memRead = 0;
        id_iu1Reg.memWrite = 0;
        id_iu1Reg.branchBEQ = 0;
        id_iu1Reg.branchBNE = 0;
        id_iu1Reg.jump = 0;

        id_iu1Reg.aluOp = "010";
        
        //add control
        if (funct == "100000"){
            id_iu1Reg.execute_inst = "10";
        }
        //subtract control
        else if (funct == "100010"){
            id_iu1Reg.execute_inst = "10";
        }
        //and control
        else if (funct == "100100"){
            id_iu1Reg.execute_inst = "01";
        }
        //or control
        else if (funct == "100101"){
            id_iu1Reg.execute_inst = "01";
        }
        //sll control
        else if (funct == "000000"){
            id_iu1Reg.execute_inst = "01";
        }
        //srl control
        else if (funct == "000010"){
            id_iu1Reg.execute_inst = "01";
        }
        //mult control
        else if (funct == "011000"){
            id_iu1Reg.execute_inst = "11";
        }

    }
    //andi
    else if (opcode == "001100"){

        id_iu1Reg.regDst = 0;
        id_iu1Reg.aluSrc = 1;
        id_iu1Reg.memToReg = 0;
        id_iu1Reg.regWrite = 1;
        id_iu1Reg.memRead = 0;
        id_iu1Reg.memWrite = 0;
        id_iu1Reg.branchBEQ = 0;
        id_iu1Reg.branchBNE = 0;
        id_iu1Reg.jump = 0;

        id_iu1Reg.aluOp = "011";
        id_iu1Reg.execute_inst = "01";
    }
    //ori
    else if (opcode == "001101"){

        id_iu1Reg.regDst = 0;
        id_iu1Reg.aluSrc = 1;
        id_iu1Reg.memToReg = 0;
        id_iu1Reg.regWrite = 1;
        id_iu1Reg.memRead = 0;
        id_iu1Reg.memWrite = 0;
        id_iu1Reg.branchBEQ = 0;
        id_iu1Reg.branchBNE = 0;
        id_iu1Reg.jump = 0;

        id_iu1Reg.aluOp = "011";
        id_iu1Reg.execute_inst = "01";
        
    }
    //addi opcode
    else if (opcode == "001000"){

        id_iu1Reg.regDst = 0;
        id_iu1Reg.aluSrc = 1;
        id_iu1Reg.memToReg = 0;
        id_iu1Reg.regWrite = 1;
        id_iu1Reg.memRead = 0;
        id_iu1Reg.memWrite = 0;
        id_iu1Reg.branchBEQ = 0;
        id_iu1Reg.branchBNE = 0;
        id_iu1Reg.jump = 0;

        id_iu1Reg.aluOp = "000";
        id_iu1Reg.execute_inst = "10";

    }
    //li opcode
    else if (opcode == "001011"){

        id_iu1Reg.regDst = 0;
        id_iu1Reg.aluSrc = 1;
        id_iu1Reg.memToReg = 0;
        id_iu1Reg.regWrite = 1;
        id_iu1Reg.memRead = 0;
        id_iu1Reg.memWrite = 0;
        id_iu1Reg.branchBEQ = 0;
        id_iu1Reg.branchBNE = 0;
        id_iu1Reg.jump = 0;

        id_iu1Reg.aluOp = "000";
        id_iu1Reg.execute_inst = "01";

    }
    //subi opcode (made up)
    else if (opcode == "100001"){

        id_iu1Reg.regDst = 0;
        id_iu1Reg.aluSrc = 1;
        id_iu1Reg.memToReg = 0;
        id_iu1Reg.regWrite = 1;
        id_iu1Reg.memRead = 0;
        id_iu1Reg.memWrite = 0;
        id_iu1Reg.branchBEQ = 0;
        id_iu1Reg.branchBNE = 0;
        id_iu1Reg.jump = 0;

        id_iu1Reg.aluOp = "001";
        id_iu1Reg.execute_inst = "10";

    }
    //lw opcode
    else if (opcode == "100011"){

        id_iu1Reg.regDst = 0;
        id_iu1Reg.aluSrc = 1;
        id_iu1Reg.memToReg = 1;
        id_iu1Reg.regWrite = 1;
        id_iu1Reg.memRead = 1;
        id_iu1Reg.memWrite = 0;
        id_iu1Reg.branchBEQ = 0;
        id_iu1Reg.branchBNE = 0;
        id_iu1Reg.jump = 0;

        id_iu1Reg.aluOp = "000";
        id_iu1Reg.execute_inst = "01";
    }
    //sw opcode
    else if (opcode == "101011"){

        id_iu1Reg.regDst = 0;
        id_iu1Reg.aluSrc = 1;
        id_iu1Reg.memToReg = 1;
        id_iu1Reg.regWrite = 0;
        id_iu1Reg.memRead = 0;
        id_iu1Reg.memWrite = 1;
        id_iu1Reg.branchBEQ = 0;
        id_iu1Reg.branchBNE = 0;
        id_iu1Reg.jump = 0;

        id_iu1Reg.aluOp = "000";
        id_iu1Reg.execute_inst = "01";
    }
    //slli
    else if (opcode == "010010"){

        id_iu1Reg.regDst = 0;
        id_iu1Reg.aluSrc = 1;
        id_iu1Reg.memToReg = 0;
        id_iu1Reg.regWrite = 1;
        id_iu1Reg.memRead = 0;
        id_iu1Reg.memWrite = 0;
        id_iu1Reg.branchBEQ = 0;
        id_iu1Reg.branchBNE = 0;
        id_iu1Reg.jump = 0;

        id_iu1Reg.aluOp = "100";
        id_iu1Reg.execute_inst = "01";
        
    }
    //srli
    else if (opcode == "000011"){

        id_iu1Reg.regDst = 0;
        id_iu1Reg.aluSrc = 1;
        id_iu1Reg.memToReg = 0;
        id_iu1Reg.regWrite = 1;
        id_iu1Reg.memRead = 0;
        id_iu1Reg.memWrite = 0;
        id_iu1Reg.branchBEQ = 0;
        id_iu1Reg.branchBNE = 0;
        id_iu1Reg.jump = 0;

        id_iu1Reg.aluOp = "101";
        id_iu1Reg.execute_inst = "01";
        
    }
    //beq
    else if (opcode == "000100"){

        id_iu1Reg.regDst = 0;
        id_iu1Reg.aluSrc = 0;
        id_iu1Reg.memToReg = 0;
        id_iu1Reg.regWrite = 0;
        id_iu1Reg.memRead = 0;
        id_iu1Reg.memWrite = 0;
        id_iu1Reg.branchBEQ = 1;
        id_iu1Reg.branchBNE = 0;
        id_iu1Reg.jump = 0;

        id_iu1Reg.aluOp = "001";
        id_iu1Reg.execute_inst = "01";

    }
    //bne
    else if (opcode == "000101"){
        
        id_iu1Reg.regDst = 0;
        id_iu1Reg.aluSrc = 0;
        id_iu1Reg.memToReg = 0;
        id_iu1Reg.regWrite = 0;
        id_iu1Reg.memRead = 0;
        id_iu1Reg.memWrite = 0;
        id_iu1Reg.branchBEQ = 0;
        id_iu1Reg.branchBNE = 1;
        id_iu1Reg.jump = 0;

        id_iu1Reg.aluOp = "001";
        id_iu1Reg.execute_inst = "01";

    }
    //j
    else if (opcode == "000010"){

        id_iu1Reg.regDst = 0;
        id_iu1Reg.aluSrc = 0;
        id_iu1Reg.memToReg = 0;
        id_iu1Reg.regWrite = 0;
        id_iu1Reg.memRead = 0;
        id_iu1Reg.memWrite = 0;
        id_iu1Reg.branchBEQ = 0;
        id_iu1Reg.branchBNE = 0;
        id_iu1Reg.jump = 1;
        
        id_iu1Reg.aluOp = "000";
        id_iu1Reg.execute_inst = "01";

    }
    //multi opcode
    else if (opcode == "001001"){

        id_iu1Reg.regDst = 0;
        id_iu1Reg.aluSrc = 1;
        id_iu1Reg.memToReg = 0;
        id_iu1Reg.regWrite = 1;
        id_iu1Reg.memRead = 0;
        id_iu1Reg.memWrite = 0;
        id_iu1Reg.branchBEQ = 0;
        id_iu1Reg.branchBNE = 0;
        id_iu1Reg.jump = 0;

        id_iu1Reg.aluOp = "111";
        id_iu1Reg.execute_inst = "11";

    }


}

string ALU(string var1, string var2, string control){
    
    //converts from binary to int
    int negativeValue1 = 1;
    if (var1[0] == '1') {

        var1.replace(0, 1, "");
        negativeValue1 = -1;

    }

    int negativeValue2 = 1;
    if (var2[0] == '1') {

        var2.replace(0, 1, "");
        negativeValue2 = -1;

    }

    int num_var1 = stoi(var1, nullptr, 2) * negativeValue1;
    int num_var2 = stoi(var2, nullptr, 2) * negativeValue2;

    //set to zero to prevent code from breaking
    int result = 0;

    //checks alu control to do alu operation
    if (control == "0010"){
        result = num_var1 + num_var2;
    }
    else if (control == "0110"){
        result = num_var1 - num_var2;
    }
    else if (control == "0000"){
        result = num_var1 & num_var2;
    }
    else if (control == "0001"){
        result = num_var1 | num_var2;
    }
    else if (control == "1111"){  
        result = num_var1 << num_var2;
    }
    else if (control == "1001"){   
        result = num_var1 >> num_var2;
    }
    else if (control == "1000"){   
        result = num_var1 * num_var2;
    }

    string makeResult;
    if (result < 0){

        makeResult = "1";
        result *= -1;

    }
    else {

        makeResult = "0";

    }

    makeResult.append(decimalToBinaryStr(result, 31));

    //returns a 32 bit answer
    return makeResult;
}

string ALUcontrol(string aluOp, string funct, string opcode){

    //r-type instruction
    if (aluOp == "010"){
        
        //add control
        if (funct == "100000"){
            return "0010";
        }
        //subtract control
        else if (funct == "100010"){
            return "0110";
        }
        //and control
        else if (funct == "100100"){
            return "0000";
        }
        //or control
        else if (funct == "100101"){
            return "0001";
        }
        //sll control
        else if (funct == "000000"){
            //custom control
            return "1111";
        }
        //srl control
        else if (funct == "000010"){
            //custom control
            return "1001";
        }
        //mult control
        else if (funct == "011000"){
            return "1000";
        }
        
    }
    //lw, sw, li, and addi control
    else if (aluOp == "000") {
        return "0010";
    }
    //subi, beq, bne control
    else if (aluOp == "001"){
        return "0110";
    }
    //made up for andi and ori
    else if (aluOp == "011"){

        //andi
        if (opcode == "001100")
            return "0000";
        //ori
        if (opcode == "001101")
            return "0001";
        
    }
    //slli
    else if (aluOp == "100"){
        return "1111";
    }
    //srli
    else if (aluOp == "101"){
        return "1001";
    }
    //multi
    else if (aluOp == "111"){
        return "1000";
    }


    //incase breaks
    return "0000";
}


void forwarding(){

    //ForwardA dectection
    if (fw_unit.mem_wbRegWrite && (fw_unit.mem_wbRegisterRd != "00000")
        && !((fw_unit.iu2_iu3RegWrite && (fw_unit.iu2_iu3RegisterRd != "00000")
        && (fw_unit.iu2_iu3RegisterRd == fw_unit.registerRs)) 
        || (fw_unit.iu1_iu2RegWrite && (fw_unit.iu1_iu2RegisterRd != "00000")
        && (fw_unit.iu1_iu2RegisterRd == fw_unit.registerRs))
        || (fw_unit.iu3_memRegWrite && (fw_unit.iu3_memRegisterRd != "00000")
        && (fw_unit.iu3_memRegisterRd == fw_unit.registerRs)))
        && (fw_unit.mem_wbRegisterRd == fw_unit.registerRs)){

        fw_unit.forwardA = "001";

    }
    else if (fw_unit.iu2_iu3RegWrite && (fw_unit.iu2_iu3RegisterRd != "00000")
        && (fw_unit.iu2_iu3RegisterRd == fw_unit.registerRs)) {

        fw_unit.forwardA = "010";

    }
    else if (fw_unit.iu1_iu2RegWrite && (fw_unit.iu1_iu2RegisterRd != "00000")
        && (fw_unit.iu1_iu2RegisterRd == fw_unit.registerRs) && fw_unit.forwardID){
        
        fw_unit.forwardA = "011";

    }
    else if (fw_unit.iu3_memRegWrite && (fw_unit.iu3_memRegisterRd != "00000")
        && (fw_unit.iu3_memRegisterRd == fw_unit.registerRs)){

        fw_unit.forwardA = "100";

    }
    else {

        fw_unit.forwardA = "000";

    }

    //ForwardB dectection
    if (fw_unit.mem_wbRegWrite && (fw_unit.mem_wbRegisterRd != "00000")
        && !((fw_unit.iu2_iu3RegWrite && (fw_unit.iu2_iu3RegisterRd != "00000")
        && (fw_unit.iu2_iu3RegisterRd == fw_unit.registerRt)) 
        || (fw_unit.iu1_iu2RegWrite && (fw_unit.iu1_iu2RegisterRd != "00000")
        && (fw_unit.iu1_iu2RegisterRd == fw_unit.registerRt))
        || (fw_unit.iu3_memRegWrite && (fw_unit.iu3_memRegisterRd != "00000")
        && (fw_unit.iu3_memRegisterRd == fw_unit.registerRt)))
        && (fw_unit.mem_wbRegisterRd == fw_unit.registerRt)){

        fw_unit.forwardB = "001";

    }
    else if (fw_unit.iu2_iu3RegWrite && (fw_unit.iu2_iu3RegisterRd != "00000")
        && (fw_unit.iu2_iu3RegisterRd == fw_unit.registerRt)) {

        fw_unit.forwardB = "010";

    }
    else if (fw_unit.iu1_iu2RegWrite && (fw_unit.iu1_iu2RegisterRd != "00000")
        && (fw_unit.iu1_iu2RegisterRd == fw_unit.registerRt) && fw_unit.forwardID){
        
        fw_unit.forwardB = "011";

    }
    else if (fw_unit.iu3_memRegWrite && (fw_unit.iu3_memRegisterRd != "00000")
        && (fw_unit.iu3_memRegisterRd == fw_unit.registerRt)){

        fw_unit.forwardB = "100";

    }
    else {

        fw_unit.forwardB = "000";

    }

}


void hazardDetection(){

    //load use detection and forward detection, stalls here
    if ((hazard_unit.id_iu1MemRead && ((hazard_unit.id_iu1RegisterRt == hazard_unit.if_idRegisterRs) || 
        (hazard_unit.id_iu1RegisterRt == hazard_unit.if_idRegisterRt))) || 
        (hazard_unit.iu1_iu2MemRead && ((hazard_unit.iu1_iu2RegisterRt == hazard_unit.if_idRegisterRs) || 
        (hazard_unit.iu1_iu2RegisterRt == hazard_unit.if_idRegisterRt))) || 
        (hazard_unit.iu2_iu3MemRead && ((hazard_unit.iu2_iu3RegisterRt == hazard_unit.if_idRegisterRs) || 
        (hazard_unit.iu2_iu3RegisterRt == hazard_unit.if_idRegisterRt)))){

        hazard_unit.stall = 1;
        hazard_unit.PCWrite = 0;
    }
    else if (hazard_unit.iu1_iu2RegWrite && (hazard_unit.iu1_iu2RegisterRd != "00000")
        && ((hazard_unit.iu1_iu2RegisterRd == hazard_unit.if_idRegisterRt) 
        || (hazard_unit.iu1_iu2RegisterRd == hazard_unit.if_idRegisterRs)) && (!hazard_unit.iu1executed)) {

        hazard_unit.stall = 1;
        hazard_unit.PCWrite = 0;

    }
    else if (hazard_unit.iu2_iu3RegWrite && (hazard_unit.iu2_iu3RegisterRd != "00000")
        && ((hazard_unit.iu2_iu3RegisterRd == hazard_unit.if_idRegisterRt) 
        || (hazard_unit.iu2_iu3RegisterRd == hazard_unit.if_idRegisterRs)) && (!hazard_unit.iu2executed)){

        hazard_unit.stall = 1;
        hazard_unit.PCWrite = 0;

    }
    else {

        hazard_unit.stall = 0;
        hazard_unit.PCWrite = 1;

    }

}