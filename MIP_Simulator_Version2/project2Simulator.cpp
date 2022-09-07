/*---------------------------------------------------------------------------/
/File Name: project2Simulator.cpp
/Author:    Muhammed Muiktar
/Date:      4/23/2021
/Section:   02
/Email:     mmuktar1@umbc.edu
/
/Description: This program simulates MIPS pipline instructions
/             (No cache, hazards, single EX stage, branching)
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
const int FETCH = 1, DECODE = 2, EXECUTE = 3, ACCESS = 4, WRITE_BACK = 5;

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
void execute();

//Precondition:     Instrucion was executed    
//Post condition:   Access memory for intruction and populates registers
void accessMemory();

//Precondition:     Instrucion memory accessed    
//Post condition:   Writes to register in instructions and populates registers
void writeBack();

//-----------------------------------------------------------------------------

//Precondition:     N/A    
//Post condition:   Poplates id_ex registers with proper alu controls
void makeControl(string opcode);

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

//ID_EX Register
struct ID_EX{

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

} id_exReg;

//EXE_MEM Register
struct EX_MEM{

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

} ex_memReg;

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

struct FORWARD_UNIT{

    //units for forwarding
    bool mem_wbRegWrite = 0;
    bool ex_memRegWrite = 0;

    string mem_wbRegisterRd = "";
    string ex_memRegisterRd = "";

    string id_exRegisterRs = "";
    string id_exRegisterRt = "";

    string forwardA = "";
    string forwardB = "";

} fw_unit; 

struct HAZARD_UNIT{

    //units for hazard detection
    bool stall = 0;
    bool id_exMemRead = 0;
    bool PCWrite = 1;
    
    string id_exRegisterRt = "";
    
    string if_idRegisterRs = "";
    string if_idRegisterRt = "";

} hazard_unit;

int main(int argc, char *argv[]){

    try {
        
        //Number of arguments are not correct, needs four to execute program
        if (argc != 4) {

            throw "Format must be simulatorV1 <instructions> <data> <output>";
                
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

    //Insert I type instructions, instruction - opcode
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("LW", "100011"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("SW", "101011"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("LI", "001000")); 
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("ADDI", "001000"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("SUBI", "100001")); 
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("ANDI", "001100"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("ORI", "001101"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("BEQ", "000100"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("BNE", "000101"));
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("SLLI", "010010")); 
    I_INSTRUCTION_LIST.push_back(tuple<string,string> ("SRLI", "000011"));


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

        if (getInst == "J" || getInst == "HLT")
            size++;
        
    }

    //resetting ifstream
    loadInst.close();
    loadInst.open(filename);

    string getReg1, getReg2, getReg3, getCons, getAddr;
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
                
                //clears spaces
                for (int i = 0; i < int(getReg1.size()); i++){
                    if (isspace(getReg1[i])){
                        getReg1.replace(i, 1 , "");
                    }
                }

                //gets register2
                getline(loadInst, getReg2, ',');

                //clears spaces
                for (int i = 0; i < int(getReg2.size()); i++){
                    if (isspace(getReg2[i])){
                        getReg2.replace(i, 1 , "");
                    }
                }

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

                //clears spaces
                for (int i = 0; i < int(getReg1.size()); i++){
                    if (isspace(getReg1[i])){
                        getReg1.replace(i, 1 , "");
                    }
                }

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
                    
                    //clears spaces
                    for (int i = 0; i < int(getReg2.size()); i++){
                        if (isspace(getReg2[i])){
                            getReg2.replace(i, 1 , "");
                        }
                    }

                }
                else {
                    
                    //gets register2
                    getline(loadInst, getReg2, ',');

                    //clears spaces
                    for (int i = 0; i < int(getReg2.size()); i++){
                        if (isspace(getReg2[i])){
                         getReg2.replace(i, 1 , "");
                        }
                    }
                    
                }

                if (getInst == "LW" || getInst == "SW") {
                    //gets constant for lw or sw 
                    getCons = temp.substr(0, temp.find("("));

                    //clears spaces
                    for (int i = 0; i < int(getCons.size()); i++){
                        if (isspace(getCons[i])){
                            getCons.replace(i, 1 , "");
                        }
                    }
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

                        //adds constant
                        tempBinary.append(decimalToBinaryStr(stoi(getCons), 16));
                    
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
        if (getInst == "HLT") 
            list_0x00.push_back("11111100000000000000000000000000");
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

void writeOutput(string filename){

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
        
        outInst.width(64);
        outInst << right << "IF   ID   EX   MEM  WB";
        outInst << "\n";


    }
    //first flushing is written to output file
    else if (!firstFlush) {

        outInst.width(64);
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
                
                //insert label with instruction
                outInst.width(10); 
                outInst << left << getFirst;
                
                
                loadInst >> getFirst;
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
                outInst.width(22);
                outInst << left << "";
            
            } 
            else {

                //load register1
                getline(loadInst, getInst, ',');

                //clears spaces
                for (int i = 0; i < int(getInst.size()); i++){
                    if (isspace(getInst[i])){
                        getInst.replace(i, 1 , "");
                    }
                }
            
                outInst.width(5);
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

                    //clears spaces
                    for (int i = 0; i < int(getInst.size()); i++){
                        if (isspace(getInst[i])){
                            getInst.replace(i, 1 , "");
                        }
                    }

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

    //writes register values
    for (int i = 0; i < 32; i++) {

        outReg << "%"<< i << " = " 
        << decimalToBinaryStr(registers[i], 32) << endl;

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
    queueInstructions.push_back(tuple<int,int> (EXECUTE, clockCycle + 2));
    queueInstructions.push_back(tuple<int,int> (ACCESS, clockCycle + 3));
    queueInstructions.push_back(tuple<int,int> (WRITE_BACK, clockCycle + 4));

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
                else if (get<0>(*i) == EXECUTE){
                    execute();       
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
            if (id_exReg.jump) {
                
                prevPC = PC;
                PC = id_exReg.jumpAddress;
                if_idReg.fetchedInstruction = list_0x00[PC];
               
            }
            else if (pcSrc) {

                prevPC = PC;
                PC = id_exReg.nextAddress;
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

    //hazard not detected
    if (!hazard_unit.stall)
        trackIFClock.push_back(clockCycle);

    //flush detected
    if (ifFlush) {
        
        if_idReg.writeFlushed = 1;

        holdAddress = if_idReg.nextAddress;
        holdPrevPC = prevPC;
        
        //flushes with a nop instruction
        if_idReg.fetchedInstruction = "00000000000000000000000000000000";
        
        //nops
        id_exReg.idNOP = 1;
        id_exReg.exNOP = 1;
        id_exReg.memNOP = 1;
        id_exReg.wbNOP = 1;

    }
    else {

        if_idReg.writeFlushed = 0;
    
    }

    
}

void instructionDecode(){

    //updates controls id_exReg
    string opcode = if_idReg.fetchedInstruction.substr(0,6);

    //checks if halt
    if (opcode == "111111") {

        if_idReg.stopInstrucions = true;

    }
    else {

        string getFunct = if_idReg.fetchedInstruction.substr(26,6);

        //gets rs 
        string register1 = if_idReg.fetchedInstruction.substr(6,5);
        string readData1 = decimalToBinaryStr(registers[stoi(register1,nullptr,2)], 32);
        id_exReg.readData1 = readData1;

        //gets rt
        string register2 = if_idReg.fetchedInstruction.substr(11,5);
        string readData2 = decimalToBinaryStr(registers[stoi(register2,nullptr,2)], 32);
        id_exReg.readData2 = readData2;

        //gets rd in R format
        id_exReg.instrutction15_11 = if_idReg.fetchedInstruction.substr(16,5);

        //rd in I format
        id_exReg.instrutction20_16 = if_idReg.fetchedInstruction.substr(11,5);

        //hazard checking
        hazard_unit.if_idRegisterRs = register1;
        hazard_unit.if_idRegisterRt = register2;

        hazardDetection();

        //hazarded detected
        if (hazard_unit.stall) {

            //sets controls to 0
            id_exReg.regDst = 0;
            id_exReg.aluSrc = 0;
            id_exReg.memToReg = 0;
            id_exReg.regWrite = 0;
            id_exReg.memRead = 0;
            id_exReg.memWrite = 0;
            id_exReg.branchBEQ = 0;
            id_exReg.branchBNE = 0;
            id_exReg.jump = 0;

            id_exReg.aluOp = "000";

            id_exReg.idNOP = 1;
            id_exReg.exNOP = 1;
            id_exReg.memNOP = 1;
            id_exReg.wbNOP = 1;

        } else {

            //populates control values in id_ex register
            makeControl(opcode);

            //not flushing
            if (!ifFlush) {
                id_exReg.idNOP = 0;
                id_exReg.exNOP = 0;
                id_exReg.memNOP = 0;
                id_exReg.wbNOP = 0;
            }
            

        }

        string tempString = if_idReg.fetchedInstruction.substr(16,16);
        int tempConst;

        //if lw or sw
        if (opcode == "100011" || opcode == "101011") {

            //makes up word difference in address
            tempConst = stoi(tempString,nullptr,2);
            tempConst /= 4;
            id_exReg.instrutction15_0 = decimalToBinaryStr(tempConst, 16);

        } 
        else {
            
            //gets last 16 bits for constant/address
            id_exReg.instrutction15_0 = if_idReg.fetchedInstruction.substr(16,16);

        }

        //shifts constant/address to 32 bits
        while (int(id_exReg.instrutction15_0.size()) != 32)
                id_exReg.instrutction15_0.insert(0, "0");

        //calucates xor
        int checkZero = stoi(readData1, nullptr, 2) ^ stoi(readData2, nullptr, 2);
        
        bool zero;
        //checks if result is zero
        if (checkZero == 0)
            zero = 1;
        else
            zero = 0;
        
        //checks for branch or jumping flush
        if ((id_exReg.branchBEQ && zero) || 
            (id_exReg.branchBNE && !zero) || id_exReg.jump) {
            
            //if jump instruction
            if (id_exReg.jump)
                pcSrc = 0;
            else
                pcSrc = 1;

            //flush
            ifFlush = 1;

            //nops
            id_exReg.exNOP = 1;
            id_exReg.memNOP = 1;
            id_exReg.wbNOP = 1;
        
        }
        //checks for branching without flushing
        else if (id_exReg.branchBEQ || id_exReg.branchBNE) {
            
            pcSrc = 0;
            ifFlush = 0;

            //nops
            id_exReg.exNOP = 1;
            id_exReg.memNOP = 1;
            id_exReg.wbNOP = 1;

        }
        //normal execution
        else {

            pcSrc = 0;
            ifFlush = 0;

        }

        //checks for branching or jumping (to track clock)
        if (id_exReg.branchBEQ || id_exReg.branchBNE || id_exReg.jump)
            id_exReg.branch_jump = 1;
        else
            id_exReg.branch_jump = 0;

        //jump and conditional addresses
        int indexAddress = stoi(id_exReg.instrutction15_0 , nullptr, 2);
        id_exReg.nextAddress = indexAddress;
        id_exReg.jumpAddress = stoi(if_idReg.fetchedInstruction.substr(6, 26), nullptr, 2);


        id_exReg.opcode = opcode;

        id_exReg.registerRs = register1;
        id_exReg.registerRt = register2;

    }

    id_exReg.writeFlushed = if_idReg.writeFlushed;

    //tracks with the right conditions
    if (!hazard_unit.stall && !id_exReg.idNOP)
        trackIDClock.push_back(clockCycle);
    else if (id_exReg.branch_jump || id_exReg.writeFlushed)
        trackIDClock.push_back(0);
}
void execute(){

    string getFunct = id_exReg.instrutction15_0.substr(26,6);

    //gets alu control
    string getAluControl = ALUcontrol(id_exReg.aluOp, getFunct, id_exReg.opcode);
    string getALUResult;
    
    string dataA, dataB;

    //forwarding
    fw_unit.id_exRegisterRs = id_exReg.registerRs;
    fw_unit.id_exRegisterRt = id_exReg.registerRt;
    
    forwarding();

    //mux created with forwarding
    if (fw_unit.forwardA == "10"){
        dataA = ex_memReg.aluResult;
    }
    else if (fw_unit.forwardA == "01"){
        dataA = mem_wbReg.muxALU;  
    }
    else if (fw_unit.forwardA == "00"){
        dataA = id_exReg.readData1;
    }

    if (fw_unit.forwardB == "10"){
        dataB = ex_memReg.aluResult;
    }
    else if (fw_unit.forwardB == "01"){
        dataB = mem_wbReg.muxALU;  
    }
    else if (fw_unit.forwardB == "00"){
        dataB = id_exReg.readData2;
    }
    
    //hazard
    hazard_unit.id_exMemRead = id_exReg.memRead;
    hazard_unit.id_exRegisterRt = id_exReg.registerRt;

    //check alusrc and gets result
    if (id_exReg.aluSrc) 
        getALUResult = ALU(dataA, id_exReg.instrutction15_0, getAluControl);

    else 
        getALUResult = ALU(dataA, dataB, getAluControl);

    
    //checks register destination to exe_mem register
    if (id_exReg.regDst)
        ex_memReg.regDestination = id_exReg.instrutction15_11;
    else
        ex_memReg.regDestination = id_exReg.instrutction20_16;

    //passes to exe_mem register
    ex_memReg.aluResult = getALUResult;
    ex_memReg.readData2 = dataB;

    ex_memReg.memWrite = id_exReg.memWrite;
    ex_memReg.memRead = id_exReg.memRead;

    ex_memReg.regWrite = id_exReg.regWrite;
    ex_memReg.memToReg = id_exReg.memToReg;


    ex_memReg.memNOP = id_exReg.memNOP;
    ex_memReg.wbNOP = id_exReg.wbNOP;

    ex_memReg.writeFlushed = id_exReg.writeFlushed;
    ex_memReg.branch_jump = id_exReg.branch_jump;

    //tracks with the right conditions
    if (!id_exReg.exNOP)
        trackEXClock.push_back(clockCycle);
    else if (id_exReg.branch_jump || id_exReg.writeFlushed)
        trackEXClock.push_back(0);
}
void accessMemory(){

    string getAddress = ex_memReg.aluResult;
    string getWriteData = ex_memReg.readData2;

    mem_wbReg.readDataMem = "";

    //checks to write to memory
    if (ex_memReg.memWrite) 

        //changes data, makes up for index at 0x100
        list_0x100[stoi(getAddress,nullptr,2) - 256] = getWriteData;

    //checks to read from address
    if (ex_memReg.memRead) {

        //passes whats read to mem_wb register
        mem_wbReg.readDataMem = list_0x100[stoi(getAddress,nullptr,2) - 256];
       
    }

    //passes to mem_wb register
    mem_wbReg.aluResult = ex_memReg.aluResult;
    mem_wbReg.regDestination = ex_memReg.regDestination;

    mem_wbReg.regWrite = ex_memReg.regWrite;
    mem_wbReg.memToReg = ex_memReg.memToReg;

    //forwarding
    fw_unit.ex_memRegisterRd = ex_memReg.regDestination;
    fw_unit.ex_memRegWrite = ex_memReg.regWrite;
    
    //nops
    mem_wbReg.wbNOP = ex_memReg.wbNOP;

    //tracks flushing
    mem_wbReg.writeFlushed = ex_memReg.writeFlushed;

    //tracks branch and jumping
    mem_wbReg.branch_jump = ex_memReg.branch_jump;

    //tracks with the right conditions
    if (!ex_memReg.memNOP)
        trackMEMClock.push_back(clockCycle);
    else if (ex_memReg.branch_jump || ex_memReg.writeFlushed)
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
        
        registers[stoi(writeRegister,nullptr,2)] = stoi(writeData,nullptr,2);

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

void makeControl(string opcode){

    //r-type opcode
    if (opcode == "000000"){

        id_exReg.regDst = 1;
        id_exReg.aluSrc = 0;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;
        id_exReg.branchBEQ = 0;
        id_exReg.branchBNE = 0;
        id_exReg.jump = 0;

        id_exReg.aluOp = "010";
        
    }
    //andi
    else if (opcode == "001100"){

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;
        id_exReg.branchBEQ = 0;
        id_exReg.branchBNE = 0;
        id_exReg.jump = 0;

        id_exReg.aluOp = "011";
        
    }
    //ori
    else if (opcode == "001101"){

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;
        id_exReg.branchBEQ = 0;
        id_exReg.branchBNE = 0;
        id_exReg.jump = 0;

        id_exReg.aluOp = "011";
        
    }
    //addi opcode
    else if (opcode == "001000"){

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;
        id_exReg.branchBEQ = 0;
        id_exReg.branchBNE = 0;
        id_exReg.jump = 0;

        id_exReg.aluOp = "000";

    }
    //subi opcode (made up)
    else if (opcode == "100001"){

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;
        id_exReg.branchBEQ = 0;
        id_exReg.branchBNE = 0;
        id_exReg.jump = 0;

        id_exReg.aluOp = "001";

    }
    //lw opcode
    else if (opcode == "100011"){

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 1;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 1;
        id_exReg.memWrite = 0;
        id_exReg.branchBEQ = 0;
        id_exReg.branchBNE = 0;
        id_exReg.jump = 0;

        id_exReg.aluOp = "000";
    }
    //sw opcode
    else if (opcode == "101011"){

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 1;
        id_exReg.regWrite = 0;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 1;
        id_exReg.branchBEQ = 0;
        id_exReg.branchBNE = 0;
        id_exReg.jump = 0;

        id_exReg.aluOp = "000";
    }
    //slli
    else if (opcode == "010010"){

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;
        id_exReg.branchBEQ = 0;
        id_exReg.branchBNE = 0;
        id_exReg.jump = 0;

        id_exReg.aluOp = "100";
        
    }
    //srli
    else if (opcode == "000011"){

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 1;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 1;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;
        id_exReg.branchBEQ = 0;
        id_exReg.branchBNE = 0;
        id_exReg.jump = 0;

        id_exReg.aluOp = "101";
        
    }
    //beq
    else if (opcode == "000100"){

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 0;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 0;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;
        id_exReg.branchBEQ = 1;
        id_exReg.branchBNE = 0;
        id_exReg.jump = 0;

        id_exReg.aluOp = "001";

    }
    //bne
    else if (opcode == "000101"){
        
        id_exReg.regDst = 0;
        id_exReg.aluSrc = 0;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 0;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;
        id_exReg.branchBEQ = 0;
        id_exReg.branchBNE = 1;
        id_exReg.jump = 0;

        id_exReg.aluOp = "001";
    }
    //j
    else if (opcode == "000010"){

        id_exReg.regDst = 0;
        id_exReg.aluSrc = 0;
        id_exReg.memToReg = 0;
        id_exReg.regWrite = 0;
        id_exReg.memRead = 0;
        id_exReg.memWrite = 0;
        id_exReg.branchBEQ = 0;
        id_exReg.branchBNE = 0;
        id_exReg.jump = 1;
        
        id_exReg.aluOp = "000";
    }


}

string ALU(string var1, string var2, string control){
    
    //converts from binary to int
    int num_var1 = stoi(var1, nullptr, 2);
    int num_var2 = stoi(var2, nullptr, 2);

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
    
    //returns a 32 bit answer
    return decimalToBinaryStr(result, 32);
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
        
    }
    //lw, sw, and addi control
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


    //incase breaks
    return "0000";
}

void forwarding(){

    //ForwardA dectection
    if (fw_unit.mem_wbRegWrite && (fw_unit.mem_wbRegisterRd != "00000")
        && !(fw_unit.ex_memRegWrite && (fw_unit.ex_memRegisterRd != "00000")
        && (fw_unit.ex_memRegisterRd == fw_unit.id_exRegisterRs))
        && (fw_unit.mem_wbRegisterRd == fw_unit.id_exRegisterRs)){

        fw_unit.forwardA = "01";

    }
    else if (fw_unit.ex_memRegWrite && (fw_unit.ex_memRegisterRd != "00000")
            && (fw_unit.ex_memRegisterRd == fw_unit.id_exRegisterRs)) {

        fw_unit.forwardA = "10";

    }
    else {

        fw_unit.forwardA = "00";

    }

    //ForwardB dectection
    if (fw_unit.mem_wbRegWrite && (fw_unit.mem_wbRegisterRd != "00000")
        && !(fw_unit.ex_memRegWrite && (fw_unit.ex_memRegisterRd != "00000")
        && (fw_unit.ex_memRegisterRd == fw_unit.id_exRegisterRt))
        && (fw_unit.mem_wbRegisterRd == fw_unit.id_exRegisterRt)){

        fw_unit.forwardB = "01";

    }
    else if (fw_unit.ex_memRegWrite && (fw_unit.ex_memRegisterRd != "00000")
            && (fw_unit.ex_memRegisterRd == fw_unit.id_exRegisterRt)) {

        fw_unit.forwardB = "10";

    }
    else {

        fw_unit.forwardB = "00";

    }

}

void hazardDetection(){

    //load use detection, stalls here
    if (hazard_unit.id_exMemRead && ((hazard_unit.id_exRegisterRt == hazard_unit.if_idRegisterRs) ||
        (hazard_unit.id_exRegisterRt == hazard_unit.if_idRegisterRt))){

        hazard_unit.stall = 1;
        hazard_unit.PCWrite = 0;

    }
    else {

        hazard_unit.stall = 0;
        hazard_unit.PCWrite = 1;

    }

}