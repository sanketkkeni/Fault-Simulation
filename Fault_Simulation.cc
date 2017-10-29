// ESE-549 Project 2 

#include <iostream>
#include <fstream> 
#include <vector>
#include <queue>
#include <time.h>
#include <stdio.h>
#include "parse_bench.tab.h"
#include "ClassCircuit.h"
#include "ClassGate.h"
#include <limits>
#include <stdlib.h>
#include <time.h>

using namespace std;

// Just for the parser
extern "C" int yyparse();
extern FILE *yyin; // Input file for parser

// Our circuit. We declare this external so the parser can use it more easily.
extern Circuit* myCircuit;

void printUsage();
vector<char> constructInputLine(string line);

//----------------------------
// If you add functions, please add the prototypes here.
void simGateValue(Gate* g);
char evalGate(vector<char> in, int c, int i);
char EvalXORGate(vector<char> in, int inv);
int LogicNot(int logicVal);
char evalFault(Gate* g, char gateValue);
//-----------------------------


int main(int argc, char* argv[]) {

  // Check the command line input and usage
  if (argc != 5) {
    printUsage();    
    return 1;
  }
  
  // Parse the bench file and initialize the circuit. (Using C style for our parser.)
  FILE *benchFile = fopen(argv[1], "r");
  if (benchFile == NULL) {
    cout << "ERROR: Cannot read file " << argv[1] << " for input" << endl;
    return 1;
  }
  yyin=benchFile;
  yyparse();
  fclose(benchFile);

  myCircuit->setupCircuit(); 
  cout << endl;

  // Setup the output text file
  ofstream outputStream;
  outputStream.open(argv[2]);
  if (!outputStream.is_open()) {
    cout << "ERROR: Cannot open file " << argv[2] << " for output" << endl;
    return 1;
  }
  
  ifstream faultStream;
  string faultLocStr;
  faultStream.open(argv[3]);
  if (!faultStream.is_open()) {
    cout << "ERROR: Cannot open fault file " << argv[3] << " for input" << endl;
    return 1;
  }
  
  // For each line in our fault file...
  while(getline(faultStream, faultLocStr)) {
      
    // Clear the old fault
    myCircuit->clearFaults();

    string faultTypeStr;
    // If we cannot get the second line in the fault pair, fail.
    if (!(getline(faultStream, faultTypeStr))) {
      break;
    }
      
    char faultType = atoi(faultTypeStr.c_str());

    // If == -1, then we are asking for fault-free simulation. 
    // Otherwise, set the fault in the circuit at the correct place.
    if (faultLocStr != "-1") {
      Gate* faultLocation = myCircuit->findGateByName(faultLocStr);      
      faultLocation->set_faultType(faultType);      
    }
    
    outputStream << "--" << endl;
    
    // Open the input vector file 
    ifstream inputStream;  
    string inputLine;  
    inputStream.open(argv[4]);
    if (!inputStream.is_open()) {
      cout << "ERROR: Cannot read file " << argv[4] << " for input" << endl;
      return 1;
    }
    
    myCircuit->printAllGates(); 
    // Try to read a line of inputs from the file.
    while(getline(inputStream, inputLine)) {
      
      // Clear logic values in my circuit and set new values
      // If there is a fault on the PI, the setPIValues() function
      // will automatically check if the value must be changed to D or D'.
      myCircuit->clearGateValues();
      myCircuit->setPIValues(constructInputLine(inputLine));
      
      /////////////////////////////////////////////////////////////////////////////
      // Write your code here.
      // At this point, the system has set up the data structure (like in Proj. 1) and 
      // set the input values for the PI gates (like in Proj. 1).
      // It has additionally used the new capability for placing faults on gate outputs
      // to set the fault in the appropriate gate.

			 vector<Gate*> circuitPOs = myCircuit->getPOGates();
				for (int i=0; i < circuitPOs.size(); i++) {
				simGateValue(circuitPOs[i]);
				}

			//cout<<circuitPOs.size();


      
      
      // Stop writing your code here.
      ////////////////////////////////////////////////////
      
      // For each test vector, print the outputs and then the faults detectable at each gate.
      vector<Gate*> outputGates = myCircuit->getPOGates();
      for (int i=0; i < outputGates.size(); i++) {
		outputStream << outputGates[i]->printValue();
      }
      outputStream << endl;      
      
    }
    inputStream.close();
    
  }
  faultStream.close();
  
  // close the output and fault streams
  outputStream.close();
   
  return 0;
}

// Print usage information (if user provides incorrect input.
void printUsage() {
  cout << "Usage: ./faultsim [bench_file] [output_loc] [fault_file] [input_vectors]" << endl << endl;
  cout << "   bench_file:    the target circuit in .bench format" << endl;
  cout << "   output_loc:    location for output file" << endl;
  cout << "   fault_file:    faults to be simulated" << endl;
  cout << "   input_vectors: list of input vectors to simulate" << endl; 
  cout << endl;
  cout << "   Simulate each vector in input_vectors for each fault in fault_file." << endl;
  cout << "   For fault free simulation, enter -1 for the fault location and type in fault_file." << endl;
  cout << endl;
}

// Just used to parse in the values from the input file.
vector<char> constructInputLine(string line) {
  
  vector<char> inputVals;
  
  for (int i=0; i<line.size(); i++) {
    if (line[i] == '0') 
      inputVals.push_back(LOGIC_ZERO);
    
    else if (line[i] == '1') 
      inputVals.push_back(LOGIC_ONE);

    else if ((line[i] == 'X') || (line[i] == 'x')) {
      inputVals.push_back(LOGIC_X);
    }
   
    else {
      cout << "ERROR: Do not recognize character " << line[i] << " in input vector file." << endl;
      assert(false);
    }
  }  
  return inputVals;
}




////////////////////////////////////////////////////////////////////////////
// Place any new functions you add here, between these two bars.


void simGateValue(Gate* g) { 

  // If this gate has an already-set value, you are done.
  if (g->getValue() != LOGIC_UNSET)//Logic Set
    return;
  
  // Recursively call this function on this gate's predecessors to
  // ensure that their values are known.
  vector<Gate*> pred = g->get_gateInputs();
  for (int i=0; i<pred.size(); i++) {
    simGateValue(pred[i]);
  }
  
  // For convenience, create a vector of the values of this
  // gate's inputs.
  vector<char> inputVals;   
  for (int i=0; i<pred.size(); i++) {
    inputVals.push_back(pred[i]->getValue());      
  }
  

  char gateType = g->get_gateType();

  char gateValue;// 
//if (g->get_faultType()==FAULT_SA1) 
	//cout<<g->get_outputName()<<endl;
  // Now, set the value of this gate based on its logical function and its input values
  switch(gateType) {   
  case GATE_NAND: {   gateValue = evalGate(inputVals, 0, 1);gateValue=evalFault(g, gateValue);
  break; }
  case GATE_NOR: { gateValue = evalGate(inputVals, 1, 1);gateValue=evalFault(g, gateValue);
 break; }
  case GATE_AND: { gateValue = evalGate(inputVals, 0, 0);gateValue=evalFault(g, gateValue);
 break; }
  case GATE_OR: { gateValue = evalGate(inputVals, 1, 0);gateValue=evalFault(g, gateValue);
 break; }
  case GATE_BUFF: { gateValue = inputVals[0];gateValue=evalFault(g, gateValue);
 break; }
  case GATE_NOT: { gateValue = LogicNot(inputVals[0]);gateValue=evalFault(g, gateValue);
 break; }
  case GATE_XOR: { gateValue = EvalXORGate(inputVals, 0);gateValue=evalFault(g, gateValue);
 break; }
  case GATE_XNOR: { gateValue = EvalXORGate(inputVals, 1);gateValue=evalFault(g, gateValue);
 break; }
  case GATE_PI: {gateValue = LOGIC_UNSET;gateValue=evalFault(g, gateValue);
 break; }
  case GATE_FANOUT: { gateValue = inputVals[0];

  
	gateValue=evalFault(g, gateValue);

  break; }
  default: { cout << "ERROR: Do not know how to evaluate gate type " << gateType << endl; assert(false);}
  }    

  // After I have calculated this gate's value, set it on the data structure.
  g->setValue(gateValue);
  
}


//--------
// Evaluate a gate value (for non-XOR gates).
//    vector<int> in: logic values of the gate's inputs
//    int c: the controlling value of this gate (e.g., 0 for AND, 1 for OR)
//    int i: 1 if this gate is inverting (e.g., NAND), 0 if not inverting (e.g., AND)
char evalGate(vector<char> in, int c, int i) {

  // Are any of the inputs of this gate the controlling value?
  bool anyC = find(in.begin(), in.end(), c) != in.end();
  
  // Are any of the inputs of this gate unknown?
  bool anyUnknown = (find(in.begin(), in.end(), LOGIC_X) != in.end());

  // Are any inputs D or Dbar?
  int anyD    = find(in.begin(), in.end(), LOGIC_D)    != in.end();
  int anyDBar = find(in.begin(), in.end(), LOGIC_DBAR) != in.end();



  // if any input is c or we have both D and D', then return c^i
  if ((anyC) || (anyD && anyDBar))
    return (i) ? LogicNot(c) : c;

  // else if any input is unknown, return unknown
  else if (anyUnknown)
    return LOGIC_X;

  // else if any input is D, return D^i
  else if (anyD)
    return (i) ? LOGIC_DBAR : LOGIC_D;

  // else if any input is D', return D'^i
  else if (anyDBar)
    return (i) ? LOGIC_D : LOGIC_DBAR;

  // else return ~(c^i)
  else
    return LogicNot((i) ? LogicNot(c) : c);
}


// Evaluate an XOR or XNOR gate
//    vector<int> in: logic values of the gate's inputs
//    int inv: 1 if this gate is inverting (XNOR), 0 if not inverting (XOR)
char EvalXORGate(vector<char> in, int inv) {

  // if any unknowns, return unknown
  bool anyUnknown = (find(in.begin(), in.end(), LOGIC_X) != in.end());
  if (anyUnknown)
    return LOGIC_X;


  // Otherwise, let's count the numbers of ones and zeros for faulty and fault-free circuits.
  // It's not required for your project, but this will work with with XOR and XNOR with > 2 inputs.
  int onesFaultFree = 0;
  int onesFaulty = 0;

  for (int i=0; i<in.size(); i++) {
    switch(in[i]) {
    case LOGIC_ZERO: {break;}
    case LOGIC_ONE: {onesFaultFree++; onesFaulty++; break;}
    case LOGIC_D: {onesFaultFree++; break;}
    case LOGIC_DBAR: {onesFaulty++; break;}
    default: {cout << "ERROR: Do not know how to process logic value " << in[i] << " in Gate::EvalXORGate()" << endl; return LOGIC_X;}
    }
  }
  
  int XORVal;

  if ((onesFaultFree%2 == 0) && (onesFaulty%2 ==0))
    XORVal = LOGIC_ZERO;
  else if ((onesFaultFree%2 == 1) && (onesFaulty%2 ==1))
    XORVal = LOGIC_ONE;
  else if ((onesFaultFree%2 == 1) && (onesFaulty%2 ==0))
    XORVal = LOGIC_D;
  else
    XORVal = LOGIC_DBAR;

  return (inv) ? LogicNot(XORVal) : XORVal;

}


// A quick function to do a logical NOT operation on the LOGIC_* macros
int LogicNot(int logicVal) {
  if (logicVal == LOGIC_ONE)
    return LOGIC_ZERO;
  if (logicVal == LOGIC_ZERO)
    return LOGIC_ONE;
  if (logicVal == LOGIC_D)
    return LOGIC_DBAR;
  if (logicVal == LOGIC_DBAR)
    return LOGIC_D;
  if (logicVal == LOGIC_X)
    return LOGIC_X;
      
  cout << "ERROR: Do not know how to invert " << logicVal << " in LogicNot(int logicVal)" << endl;
  return LOGIC_UNSET;
}

char evalFault(Gate* g, char gateValue) {
	if ((g->get_faultType()==FAULT_SA0) && (gateValue==LOGIC_ONE)) 
	{
		 gateValue=LOGIC_D;
	}
	else if ((g->get_faultType()==FAULT_SA1)&& (gateValue==LOGIC_ZERO))
	{
		 gateValue=LOGIC_DBAR;
	}
	return gateValue;
}

////////////////////////////////////////////////////////////////////////////


