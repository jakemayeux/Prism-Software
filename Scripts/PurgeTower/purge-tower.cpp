// Jake Mayeux
// July 2017
// purge-tower.cpp

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <regex>
#include <cmath>

using namespace std;

float getValue(string l, char key){
	smatch m;
	string s;
	int pos = l.find(key);
	if(pos != -1){
		s = l.substr(pos+1,13); // 13 should be enough
	}else{
		return nanf("");
	}

	regex e("^[0-9]+\\.?[0-9]*");

	if(regex_search(s, m, e)){
		return stof(m[0]);
	}
	return nanf("");
}

void drawTower(int tnum, ofstream *ouf, int epos){
	*ouf << ";T Draw Tower" << endl;
	ifstream tow("PurgeTXTs/Tower"+to_string(tnum)+".txt");
	string l;
	while(getline(tow, l)){
		if(l.find('Z') == -1){
			*ouf << l << endl;
		}
	}
	*ouf << "G92 E" << epos << endl;
}

float findLastEpos(ifstream *inf){
	int pos = inf->tellg();
	while(inf->peek() != 'E'){
		inf->unget();
	}
	string l;
	getline(*inf, l);
	//cout << l << endl;
	return stof(l.substr(1,13));
}

int main(int argc, char* argv[]){
	//cout << argv[1] << endl;
	
	ifstream inf("PurgeIn.gcode");
	ifstream twn("TowerNames.txt"); //cross platform and we dont need extra libs
	ofstream ouf("PurgeOut.gcode");

	string l;
	string towers [9][70]; // Tower files are no longer than 70 lines and we have 9 towers
	string tdir = "PurgeTXTs";
	string towerNames [9];
	string lastLine = "";

	float lastz = 0;
	float zhop = 1;
	float zpos = 0;
	float newz = 0;
	float epos = 0;
	float newe = 0;

	int seekPos = 0;
	int swaps = 0;
	int maxSwaps = 0;
	int ct = 0; // current tower

	string tn;
	string ltn = "unset";

	string gc;

	//int count = 0;
	//while(getline(twn, l)){
	//	int tl = 0;
	//	string t;
	//	towerNames[count] = l;
	//	ifstream tow(tdir+"/"+l);
	//	while(getline(tow, t)){
	//		towers[count][tl]	= t;
	//		//cout << towers[count][tl] << endl;
	//		tl++;
	//	}
	//	count++;
	//}

	//drawTower(2, &ouf);

	// find max color swaps and last z for swapping
	while(getline(inf, l)){
		gc = l.substr(0,l.find(' '));

		if(gc == "G1"){
			newz = getValue(l, 'Z');
			if(!isnan(newz) && newz - zpos < zhop - 0.01 && newz != zpos){
				// because we dont know where the z is going to start
				if(zpos != 0){
					swaps = 0;
				}
				zpos = newz;
			}
		}else if(gc.substr(0,1) == "T"){
			tn = gc;
			if(ltn == "unset"){
				ltn = tn;
				lastz = zpos;
			}else if(tn != ltn){
				ltn = tn;
				lastz = zpos;
				swaps++;
				if(swaps > maxSwaps){
					maxSwaps = swaps;
				}
			}
		}
	}
	
	cout << "max swaps " << maxSwaps << endl;
	inf.clear();
	inf.seekg(0, inf.beg);
	zpos = 0;

	// write the file and place purge towers
	while(getline(inf, l)){
		if(zpos > lastz){
			ouf << l << endl;
			continue;
		}
		gc = l.substr(0,l.find(' '));

		if(gc == "G1"){
			newz = getValue(l, 'Z');
			//newe = getValue(l, 'E');
			if(!isnan(newz) && newz - zpos < zhop - 0.01 && newz != zpos){
				
				// fill in the rest of the towers
				if(swaps < maxSwaps && zpos > 0){
					for(int i = swaps; i < maxSwaps; ++i){
						drawTower(i+1, &ouf, 0);
					}
				}
				swaps = 0;
				zpos = newz;
			}
		}else if(gc.substr(0,1) == "T"){
			tn = gc;
			if(ltn == "unset"){
				ltn = tn;
			}else if(tn != ltn){
				ltn = tn;
				swaps++;
				if(zpos > 0){
					epos = findLastEpos(&inf);
					drawTower(swaps, &ouf, epos);
				}
			}
		}
		ouf << l << endl;
	}

}

