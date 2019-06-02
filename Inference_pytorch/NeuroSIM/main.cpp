/*******************************************************************************
* Copyright (c) 2015-2017
* School of Electrical, Computer and Energy Engineering, Arizona State University
* PI: Prof. Shimeng Yu
* All rights reserved.
* 
* This source code is part of NeuroSim - a device-circuit-algorithm framework to benchmark 
* neuro-inspired architectures with synaptic devices(e.g., SRAM and emerging non-volatile memory). 
* Copyright of the model is maintained by the developers, and the model is distributed under 
* the terms of the Creative Commons Attribution-NonCommercial 4.0 International Public License 
* http://creativecommons.org/licenses/by-nc/4.0/legalcode.
* The source code is free and you can redistribute and/or modify it
* by providing that the following conditions are met:
* 
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
* 
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* Developer list: 
*   Pai-Yu Chen	    Email: pchen72 at asu dot edu 
*                    
*   Xiaochen Peng   Email: xpeng15 at asu dot edu
********************************************************************************/

#include <cstdio>
#include <random>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <vector>
#include <sstream>
#include "constant.h"
#include "formula.h"
#include "Param.h"
#include "Tile.h"
#include "Chip.h"
#include "ProcessingUnit.h"
#include "SubArray.h"
#include "Definition.h"

using namespace std;

vector<vector<double> > getNetStructure(const string &inputfile);

int main(int argc, char * argv[]) {   
	gen.seed(0);

	vector<vector<double> > netStructure;
	netStructure = getNetStructure(argv[1]);

	// define weight/input/memory precision from wrapper
	param->synapseBit = atoi(argv[2]);              // precision of synapse weight
	param->numBitInput = atoi(argv[3]);             // precision of input neural activation
	if (param->cellBit > param->synapseBit) {
		cout << "ERROR!: Memory precision is even higher than synapse precision, please modify 'cellBit' in Param.cpp!" << endl;
		param->cellBit = param->synapseBit;
	}
	param->numColPerSynapse = ceil((double)param->synapseBit/(double)param->cellBit); 
	param->numRowPerSynapse = 1;
	
	double maxPESizeNM, maxTileSizeCM, numPENM;
	vector<int> markNM;
	markNM = ChipDesignInitialize(inputParameter, tech, cell, netStructure, &maxPESizeNM, &maxTileSizeCM, &numPENM);		
	
	double desiredNumTileNM, desiredPESizeNM, desiredNumTileCM, desiredTileSizeCM, desiredPESizeCM;
	int numTileRow, numTileCol;
	
	vector<vector<double> > numTileEachLayer;
	vector<vector<double> > utilizationEachLayer;
	vector<vector<double> > speedUpEachLayer;
	vector<vector<double> > tileLocaEachLayer;
	
	numTileEachLayer = ChipFloorPlan(true, false, false, netStructure, markNM, 
					maxPESizeNM, maxTileSizeCM, numPENM, 
					&desiredNumTileNM, &desiredPESizeNM, &desiredNumTileCM, &desiredTileSizeCM, &desiredPESizeCM, &numTileRow, &numTileCol);	
					
	utilizationEachLayer = ChipFloorPlan(false, true, false, netStructure, markNM, 
					maxPESizeNM, maxTileSizeCM, numPENM,
					&desiredNumTileNM, &desiredPESizeNM, &desiredNumTileCM, &desiredTileSizeCM, &desiredPESizeCM, &numTileRow, &numTileCol);
	
	speedUpEachLayer = ChipFloorPlan(false, false, true, netStructure, markNM,
					maxPESizeNM, maxTileSizeCM, numPENM,
					&desiredNumTileNM, &desiredPESizeNM, &desiredNumTileCM, &desiredTileSizeCM, &desiredPESizeCM, &numTileRow, &numTileCol);
					
	tileLocaEachLayer = ChipFloorPlan(false, false, false, netStructure, markNM,
					maxPESizeNM, maxTileSizeCM, numPENM,
					&desiredNumTileNM, &desiredPESizeNM, &desiredNumTileCM, &desiredTileSizeCM, &desiredPESizeCM, &numTileRow, &numTileCol);
	
	cout << "------------------------------ FloorPlan --------------------------------" <<  endl;
	
	if (!param->novelMapping) {
		cout << "Desired Conventional Mapped Tile Storage Size: " << desiredTileSizeCM << "x" << desiredTileSizeCM << endl;
	} else {
		cout << "Desired Conventional Mapped Tile Storage Size: " << desiredTileSizeCM << "x" << desiredTileSizeCM << endl;
		cout << "Desired Novel Mapped Tile Storage Size: " << numPENM << "x" << desiredPESizeNM << "x" << desiredPESizeNM << endl;
	}
	
	cout << "----------------- # of tile used for each layer -----------------" <<  endl;
	double totalNumTile = 0;
	for (int i=0; i<netStructure.size(); i++) {
		cout << "layer" << i+1 << ": " << numTileEachLayer[0][i] * numTileEachLayer[1][i] << endl;
		totalNumTile += numTileEachLayer[0][i] * numTileEachLayer[1][i];
	}
	cout << endl;

	cout << "----------------- Speed-up of each layer ------------------" <<  endl;
	for (int i=0; i<netStructure.size(); i++) {
		cout << "layer" << i+1 << ": " << speedUpEachLayer[0][i] << ", " << speedUpEachLayer[1][i] << endl;
	}
	cout << endl;
	
	cout << "----------------- Utilization of each layer ------------------" <<  endl;
	double realMappedMemory = 0;
	for (int i=0; i<netStructure.size(); i++) {
		cout << "layer" << i+1 << ": " << utilizationEachLayer[i][0] << endl;
		realMappedMemory += numTileEachLayer[0][i] * numTileEachLayer[1][i] * utilizationEachLayer[i][0];
	}
	cout << "Memory Utilization of Whole Chip: " << realMappedMemory/totalNumTile << endl;
	cout << endl;
	cout << "---------------------------- FloorPlan Done ------------------------------" <<  endl;
	cout << endl;
	cout << endl;
	cout << endl;
	
	double numComputation = 0;
	for (int i=0; i<netStructure.size(); i++) {
		numComputation += netStructure[i][0] * netStructure[i][1] * netStructure[i][2] * netStructure[i][3] * netStructure[i][4] * netStructure[i][5];
	}
	
	ChipInitialize(inputParameter, tech, cell, netStructure, markNM, numTileEachLayer,
					numPENM, desiredNumTileNM, desiredPESizeNM, desiredNumTileCM, desiredTileSizeCM, desiredPESizeCM, numTileRow, numTileCol);
					
	double chipHeight, chipWidth, chipArea;
	double CMTileheight = 0;
	double CMTilewidth = 0;
	double NMTileheight = 0;
	double NMTilewidth = 0;;
						
	chipArea = ChipCalculateArea(inputParameter, tech, cell, desiredNumTileNM, numPENM, desiredPESizeNM, desiredNumTileCM, desiredTileSizeCM, desiredPESizeCM, numTileRow, 
					&chipHeight, &chipWidth, &CMTileheight, &CMTilewidth, &NMTileheight, &NMTilewidth);		
	
	double chipReadLatency = 0;
	double chipReadDynamicEnergy = 0;
	double chipLeakageEnergy = 0;
	double chipLeakage = 0;
	double chipbufferLatency = 0;
	double chipbufferReadDynamicEnergy = 0;
	double chipicLatency = 0;
	double chipicReadDynamicEnergy = 0;
	
	double layerReadLatency = 0;
	double layerReadDynamicEnergy = 0;
	double tileLeakage = 0;
	double layerbufferLatency = 0;
	double layerbufferDynamicEnergy = 0;
	double layericLatency = 0;
	double layericDynamicEnergy = 0;
	
	cout << "-------------------------------------- Hardware Performance --------------------------------------" <<  endl;
	
	for (int i=0; i<netStructure.size(); i++) {
		
		cout << "-------------------- Estimation of Layer " << i+1 << " ----------------------" << endl;
		
		ChipCalculatePerformance(cell, i, argv[2*i+4], argv[2*i+4], argv[2*i+5], netStructure[i][6],  
					netStructure, markNM, numTileEachLayer, utilizationEachLayer, speedUpEachLayer, tileLocaEachLayer,
					numPENM, desiredPESizeNM, desiredTileSizeCM, desiredPESizeCM, CMTileheight, CMTilewidth, NMTileheight, NMTilewidth,
					&layerReadLatency, &layerReadDynamicEnergy, &tileLeakage, &layerbufferLatency, &layerbufferDynamicEnergy, &layericLatency, &layericDynamicEnergy);
		
		double numTileOtherLayer = 0;
		double layerLeakageEnergy = 0;		
		for (int j=0; j<netStructure.size(); j++) {
			if (j != i) {
				numTileOtherLayer += numTileEachLayer[0][j] * numTileEachLayer[1][j];
			}
		}
		layerLeakageEnergy = numTileOtherLayer*layerReadLatency*tileLeakage;
		
		cout << "layer" << i+1 << "'s readLatency is: " << layerReadLatency*1e9 << "ns" << endl;
		cout << "layer" << i+1 << "'s readDynamicEnergy is: " << layerReadDynamicEnergy*1e12 << "pJ" << endl;
		cout << "layer" << i+1 << "'s leakageEnergy is: " << layerLeakageEnergy*1e12 << "pJ" << endl;
		cout << "layer" << i+1 << "'s buffer latency is: " << layerbufferLatency*1e9 << "ns" << endl;
		cout << "layer" << i+1 << "'s buffer readDynamicEnergy is: " << layerbufferDynamicEnergy*1e12 << "pJ" << endl;
		cout << "layer" << i+1 << "'s ic latency is: " << layericLatency*1e9 << "ns" << endl;
		cout << "layer" << i+1 << "'s ic readDynamicEnergy is: " << layericDynamicEnergy*1e12 << "pJ" << endl;
		
		chipReadLatency += layerReadLatency;
		chipReadDynamicEnergy += layerReadDynamicEnergy;
		chipLeakageEnergy += layerLeakageEnergy;
		chipLeakage += tileLeakage*numTileEachLayer[0][i] * numTileEachLayer[1][i];
		chipbufferLatency += layerbufferLatency;
		chipbufferReadDynamicEnergy += layerbufferDynamicEnergy;
		chipicLatency += layericLatency;
		chipicReadDynamicEnergy += layericDynamicEnergy;
	}
	cout << "------------------------------ Summary --------------------------------" <<  endl;
	cout << "ChipArea : " << chipArea*1e12 << "um^2" << endl;
	cout << "Chip total readLatency is: " << chipReadLatency*1e9 << "ns" << endl;
	cout << "Chip total readDynamicEnergy is: " << chipReadDynamicEnergy*1e12 << "pJ" << endl;
	cout << "Chip total leakage Energy is: " << chipLeakageEnergy*1e12 << "pJ" << endl;
	cout << "Chip buffer readLatency is: " << chipbufferLatency*1e9 << "ns" << endl;
	cout << "Chip buffer readDynamicEnergy is: " << chipbufferReadDynamicEnergy*1e12 << "pJ" << endl;
	cout << "Chip ic readLatency is: " << chipicLatency*1e9 << "ns" << endl;
	cout << "Chip ic readDynamicEnergy is: " << chipicReadDynamicEnergy*1e12 << "pJ" << endl;
	cout << endl;
	cout << "----------------------------- Performance -------------------------------" << endl;
	cout << "Energy Efficiency TOPS/W (Layer-by-Layer Process): " << numComputation/(chipReadDynamicEnergy*1e12+chipLeakageEnergy*1e12) << endl;
	cout << "Throughput FPS (Layer-by-Layer Process): " << 1/(chipReadLatency) << endl;
	cout << "-------------------------------------- Hardware Performance Done --------------------------------------" <<  endl;
	return 0;
}

vector<vector<double> > getNetStructure(const string &inputfile) {
	ifstream infile(inputfile.c_str());      // read the input file ...
	string inputline;
	string inputval;

	int ROWin=0, COLin=0;      // initialize the number of rows and columns ...
	if (!infile.good()) {        // check if the file is opened successfully 
		cerr << "Error: the input file cannot be opened!" << endl;
		exit(1);
	}else{
		while (getline(infile, inputline, '\n')) {       // at every end of each row ...
			ROWin++;                                // count the number of rows ...
		}
		infile.clear();
		infile.seekg(0, ios::beg);      // back to the begining of the file ...
		if (getline(infile, inputline, '\n')) {        // get a single row of the file ...
			istringstream iss (inputline);      // copy row values 
			while (getline(iss, inputval, ',')) {       // count the number of columns ...
				COLin++;
			}
		}	
	}
	infile.clear();
	infile.seekg(0, ios::beg);          // back to the begining of the file ...

	vector<vector<double> > netStructure;               // original data
	// load the data into inputvector ...
	for (int row=0; row<ROWin; row++) {	
		vector<double> netStructurerow;
		getline(infile, inputline, '\n');             // get one row of input string file ...
		istringstream iss;
		iss.str(inputline);
		for (int col=0; col<COLin; col++) {       
			while(getline(iss, inputval, ',')){	
				istringstream fs;
				fs.str(inputval);
				double f=0;
				fs >> f;				
				netStructurerow.push_back(f);			
			}			
		}		
		netStructure.push_back(netStructurerow);
	}
	// close the input file ...
	infile.close();
	
	return netStructure;
	netStructure.clear();
}	


