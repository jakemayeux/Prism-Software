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
#include <vector>

using namespace std;

float getValue(string l, char key){
	smatch m;
	string s;
	int pos = l.find(key);
	if(pos != -1){
		s = l.substr(pos+1,8); // 13 should be enough
	}else{
		return nanf("");
	}

	regex e("^[0-9]+\\.?[0-9]*");

	if(regex_search(s, m, e)){
		return stof(m[0]);
	}
	return nanf("");
}

vector<float> getVector(string *l, string keys){
	//cout << "getVector()" << endl;
	vector<float> ret(3);
	//ret[0] = last[0]; //defaults to the last value
	//ret[1] = last[1];
	//ret[2] = last[2];
	int cord = 0; //0: unset 1: X 2: Y 3: Z
	string num = "";
	char c;

	for(int i = 0; i < l->size(); i++){
		//cout << "asdf" << endl;
		c = l->at(i);
		//cout << c;
		if(c == 'G' || c == ' ' || c == '\r' || c == '\n'){
			if(num != ""){
				ret[cord-1] = stof(num);
				num = "";
			}
			cord = 0;
		}else if(c == keys[0]){
			cord = 1;
		}else if(c == keys[1]){
			cord = 2;
		}else if(c == keys[2]){
			cord = 3;
		}else if(cord != 0){
			num = num + c;
			//num = "5.5";
		}
	}
	//cout << endl;
	return ret;
}

string genTower(float x, float y, float r, float f, float e){ // x/y pos, radius, feedrate, flowrate
	
}

float checkVerticalIntersect(vector<vector<float>> *g1, float x, float y){
	float minSqrDist = -1;
	float dist, dx, dy;
	for(int i = 0; i < g1->size(); i++){
		//cout << g1->at(i)[0];
		dx = g1->at(i)[0]-x;
		dy = g1->at(i)[1]-y;
		dist = (dx*dx) + (dy*dy);

		if(dist < minSqrDist || minSqrDist == -1){
			minSqrDist = dist;
		}
	}
	return minSqrDist;
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
	inf->seekg(pos);
	string l;
	getline(*inf, l);
	//cout << l << endl;
	return stof(l.substr(1,13));
}

float calcEPerMM(vector<vector<float>> *path){
	float dist = 0;
	float edist = 0;
	float dx;
	float dy;
	vector<float> last;
	vector<float> v;
	for(int i = 0; i < path->size(); i++){
		v = path->at(i);
		//cout << v[0] << endl;
		//cout << v[1] << endl;
		//cout << v[2] << endl;
		if(last.size() == 0){
			last.push_back(v[0]);
			last.push_back(v[1]);
			last.push_back(v[2]);
		}else{
			dx = v[0]-last[0];
			dy = v[1]-last[1];
			dist += sqrt((dx*dx)+(dy*dy));
			edist += v[2]-last[2];
		}
	}
	return edist/dist;
}

float getEPerMM(ifstream *inf){
	int pos = inf->tellg();
	int minSpan = 10;
	int span = 0;
	vector<vector<float>> path;
	inf->seekg(0, inf->beg);
	string l;

	vector<float> v;

	while(span < minSpan){
	span = 0;
	getline(*inf, l);
		if(l.substr(0,2) == "G1"){
			if(l.find('E')){
				v = getVector(&l, "XYE");
				path.push_back(v);
				getline(*inf, l);
				while(l.find('E') != -1 && l.substr(0,2) == "G1"){
					getline(*inf, l);
					v = getVector(&l, "XYE");
					path.push_back(v);
					span++;
					//cout << v[1] << endl;
				}
				path.pop_back();
			}
		}
	}
	
	inf->seekg(pos);
	return calcEPerMM(&path);
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
	float xpos = 0;
	float ypos = 0;
	float newx = 0;
	float newy = 0;

	int seekPos = 0;
	int swaps = 0;
	int maxSwaps = 0;
	int ct = 0; // current tower

	string tn;
	string ltn = "unset";

	string gc;

	vector<vector<float>> g1; // array of all G1's for interesection detection
	vector<float> gpos(3); 
	gpos[0] = 0;
	gpos[1] = 0;
	gpos[2] = 0;

	cout << "e mm per xy mm " << getEPerMM(&inf) << endl;
	// find max color swaps and last z for swapping
	
	inf.seekg(0, inf.beg);
	while(getline(inf, l)){
		gc = l.substr(0,l.find(' '));

		if(gc == "G1"){

			//store G1s as vector for checking intersects
			gpos = getVector(&l, "XYZ");
			g1.push_back(gpos);

			//newz = gpos[2];
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

	vector<vector<float>> towerCords;
	
	for(int i = 0; towerCords.size() < maxSwaps; i++){
		
	}
	//cout << "min Sqr Dist " << checkVerticalIntersect(&g1, 100, 100) << endl;

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

