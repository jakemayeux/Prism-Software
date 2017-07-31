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

// returns the value of key given a line of GCode
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

// TODO: make return vector dependent on length of keys
// TODO: handle unfound keys
// Parses a line of GCode searching for values specified by keys
vector<float> getVector(string *l, string keys){
	vector<float> ret(3);
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

	if(num != ""){
		ret[cord-1] = stof(num);
		num = "";
	}
	//cout << endl;
	return ret;
}

// generates a slice of circular tower (no z positioning)
string genTower(float x, float y, float r, float e, float f){ // x/y pos, radius, flowrate, feedrate
	string ret = "";

	float lineWidth = 0.4;
	float dist;
	float epos = 0;
	ret = ret + "G92 E0\n";
	ret = ret + "G1 F"+to_string(f)+"\n";
	for(float i = 1; i < r; i += lineWidth){
		dist = M_PI * 2 * i;
		epos += e*dist;
		ret = ret + "G1 X" +to_string(x+i)+ " Y" +to_string(y)+ "\n";
		ret = ret + "G2 X" +to_string(x+i)+ " Y" +to_string(y)+ " I" +to_string(-i)+ " J0 E" +to_string(epos)+ "\n";
	}
	return ret;
}

// finds the distance between the model and a line perpindicular to the build plate given its x/y pos
float checkVerticalIntersect(vector<vector<float>> *g1, vector<float> *pos){
	float minSqrDist = -1;
	float dist, dx, dy;
	float x = pos->at(0);
	float y = pos->at(1);
	for(int i = 0; i < g1->size(); i++){
		//cout << g1->at(i)[0];
		dx = g1->at(i)[0]-x;
		dy = g1->at(i)[1]-y;
		dist = (dx*dx) + (dy*dy);

		if(dist < minSqrDist || minSqrDist == -1){
			minSqrDist = dist;
		}
	}
	return sqrt(minSqrDist);
}

// write the contents of Tower-N to the outfile
//void drawTower(int tnum, ofstream *ouf, int epos){
//	*ouf << ";T Draw Tower" << endl;
//	ifstream tow("PurgeTXTs/Tower"+to_string(tnum)+".txt");
//	string l;
//	while(getline(tow, l)){
//		// dont move along the Z axis
//		if(l.find('Z') == -1){
//			*ouf << l << endl;
//		}
//	}
//	*ouf << "G92 E" << epos << endl;
//}
void drawTower(string tower, ofstream *ouf, int epos){
	*ouf << ";Purge Tower" << endl;
	*ouf << tower << endl;
	*ouf << "G92 E" << epos << endl;
}

// seeks backwards until 'E' is found, then returns the value
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

// helper function for getEPerMM
// receives a sample of gcode containing G1s with X, Y and E values
// returns the flowrate per 1 mm travel
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

// returns extrusion distance per 1 mm travel
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

// polar to cartesian
vector<float> p2c(float a, float r){
	vector<float> ret;
	ret.push_back(r * cos(a));
	ret.push_back(r * sin(a));
	return ret;
}

int main(int argc, char* argv[]){
	//cout << argv[1] << endl;
	
	if(argc == 2 && argv[1] == "-h"){
		cout << "Options:" << endl;
		cout << "\t-f [outfile]" << endl;
		cout << "\t-o [infile]" << endl;
	}
	
	for(int i = 0; i < argc; i++){
		
	}

	//cout << genTower(5,7,10,100,1) << endl;

	ifstream inf("testGcode/pig.gcode");
	//ifstream twn("TowerNames.txt"); //cross platform and we dont need extra libs
	ofstream ouf("out.gcode");

	string l;
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

	vector<string> towers;
	float buildRadius = 100; //mm

	string tn;
	string ltn = "unset";

	string gc;

	vector<vector<float>> g1; // array of all G1's for interesection detection
	vector<float> gpos(3); 
	gpos[0] = 0;
	gpos[1] = 0;
	gpos[2] = 0;

	float epm = getEPerMM(&inf);
	cout << "e mm per xy mm " << epm << endl;
	
	//inf.seekg(0, inf.beg);
	
	// find max color swaps and last z for swapping
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
	cout << "last z " << lastz << endl;

	float tdist;
	float towerRadius = 5;
	float padding = 2;
	float a = padding + (2 * towerRadius);
	float b = buildRadius-towerRadius-padding;
	float angleInc = acos((2*b*b-a*a)/(2*b*b));
	vector<float> pos(2);
	vector<vector<float>> towerCords;
	for(int i = 0; i < maxSwaps; ){
		pos = p2c(angleInc*i, b);
		tdist = checkVerticalIntersect(&g1, &pos);
		if(tdist >= a){
			towerCords.push_back(pos);
			i++;
		}else if(angleInc*i > 2*M_PI){
			cout << "unabel to place towers" << endl;
			return 0;
		}
		cout << tdist << endl;
	}

	string tower;
	for(int i = 0; i < maxSwaps; i++){
		float x = towerCords[i].at(0);
		float y = towerCords[i].at(1);
		tower = genTower(x, y, towerRadius, epm, 2400);
		towers.push_back(tower);
	}
	
	//for(int i = 0; towerCords.size() < maxSwaps; i++){
		
	//}
	//cout << "min Sqr Dist " << checkVerticalIntersect(&g1, 100, 100) << endl;

	// reset so we can seek through the file from the beginning
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
						drawTower(towers.at(i), &ouf, 0);
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
				if(zpos > 0){
					epos = findLastEpos(&inf);
					drawTower(towers.at(swaps), &ouf, epos);
				}
				swaps++;
			}
		}
		ouf << l << endl;
	}

}

