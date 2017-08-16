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

float zpos = 0;
float zhop = 2;
float frate = 4800;
float retraction = 3;

// returns the value of key given a line of GCode
float getValue(string l, char key){
	smatch m;
	string s;
	//find the position of the key we want
	int pos = l.find(key);
	if(pos != -1){
		s = l.substr(pos+1,10);
	}else{
		//this returns NaN for when the value is not found
		return nanf("");
	}

	//regex for numbers with optional decimals
	regex e("^[0-9]+\\.?[0-9]*");

	if(regex_search(s, m, e)){
		//call string to float on the first match
		return stof(m[0]);
	}
	//else return NaN
	return nanf("");
}

// Parses a line of GCode searching for values specified by keys
vector<float> getVector(string *l, string keys){
	vector<float> ret(3);
	//cord tells us which key we are currently on
	int cord = 0; //0: unset 1: X 2: Y 3: Z
	string num = "";
	char c;

	//iterate through each chacter in the line
	for(int i = 0; i < l->size(); i++){
		c = l->at(i);
		//reset cord and store any values
		if(c == 'G' || c == ' ' || c == '\r' || c == '\n'){
			//convert to float and store value in the return vector at cord-1
			if(num != ""){
				ret[cord-1] = stof(num);
				num = "";
			}
			cord = 0;
		//else if we are on a key set cord to the appropriate value
		}else if(c == keys[0]){
			cord = 1;
		}else if(c == keys[1]){
			cord = 2;
		}else if(c == keys[2]){
			cord = 3;
		}else if(cord != 0){
			num = num + c;
		}
	}

	//same code as above to store the number in the vector
	if(num != ""){
		ret[cord-1] = stof(num);
		num = "";
	}
	return ret;
}

// generates a slice of circular tower (no z positioning)
string genTower(float x, float y, float r, float e, float f){ // x/y pos, radius, flowrate, feedrate
	string ret = "";

	float lineWidth = 0.7;
	float dist;
	float epos = 0;
	ret = ret + "G92 E0\n";
	ret = ret + "G1 E-"+to_string(retraction)+"\n";
	ret = ret + "G1 F"+to_string(f)+"\n";
	ret = ret + "G1 X" +to_string(x+1)+ " Y" +to_string(y)+ "\n";
	ret = ret + "G1 E0\n";
	//ret = ret + "G1 E-1\n";
	float i;
	for(i = 1; i < r; i += lineWidth){
		dist = M_PI * 2 * i;
		epos += e*dist;
		ret = ret + "G1 X" +to_string(x+i)+ " Y" +to_string(y)+ "\n";
		ret = ret + "G2 X" +to_string(x+i)+ " Y" +to_string(y)+ " I" +to_string(-i)+ " J0 E" +to_string(epos)+ "\n";
	}
	i -= lineWidth;
	ret = ret + "G2 X" +to_string(x+i)+ " Y" +to_string(y)+ " I" +to_string(-i)+ " J0\n";
	//ret = ret + "G1 E" +to_string(epos-retraction)+ "\n";
	return ret;
}

// finds the distance between the model and a line perpindicular to the build plate given its x/y pos
float checkVerticalIntersect(vector<vector<float>> *g1, vector<float> *pos){
	float minSqrDist = -1;
	float dist, dx, dy;
	float x = pos->at(0);
	float y = pos->at(1);
	for(int i = 0; i < g1->size(); i++){
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
void drawTower(string tower, ofstream *ouf, float epos, bool inHop){
	//*ouf << "G92 E0" << endl;
	//*ouf << "G1 E-5" << endl;
	if(inHop){
		*ouf << ";Unhop" << endl;
		*ouf << "G1 Z" << zpos << endl;
	}
	*ouf << ";Purge Tower" << endl;
	*ouf << tower << endl;
	if(inHop){
		*ouf << ";Rehop" << endl;
		*ouf << "G1 Z" << zpos + zhop << endl;
	}
	//set the epos back to what it was before so we dont end up over extruding
	*ouf << "G92 E" << epos << endl;
}

// seeks backwards until 'E' is found, then returns the value
float findLastEpos(ifstream *inf){
	//cout << "pos" << endl;
	//cout << inf->tellg() << endl;
	int pos = inf->tellg();
	while(inf->peek() != 'E'){
		inf->unget();
	}
	string l;
	getline(*inf, l);
	inf->seekg(pos);
	//cout << "138 stof" << l.substr(1,13) << endl;
	//cout << l << endl;
	return getValue(l, 'E');
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
		last = path->at(i);
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
	path.clear();
	getline(*inf, l);
		if(l.substr(0,2) == "G1"){
			if(l.find('E')){
				v = getVector(&l, "XYE");
				path.push_back(v);
				getline(*inf, l);
				while(l.find('E') != string::npos && l.substr(0,2) == "G1"){
					getline(*inf, l);
					v = getVector(&l, "XYE");
					path.push_back(v);
					span++;
				}
				path.pop_back();
			}
		}
	}
	
	cout << "e per mm calc span: " << span << endl;
	inf->seekg(pos);
	return calcEPerMM(&path)/1;
}

// polar to cartesian
vector<float> p2c(float a, float r){
	vector<float> ret;
	ret.push_back(r * cos(a));
	ret.push_back(r * sin(a));
	return ret;
}

int main(int argc, char* argv[]){
	//ifstream inf("testGcode/pig.gcode");
	//ifstream twn("TowerNames.txt"); //cross platform and we dont need extra libs

	string infile = "testGcode/pig.gcode";
	string outfile = "out.gcode";
	//ifstream inf("testGcode/pig.gcode");
	//ofstream ouf("out.gcode");
	string l;
	string towerNames [9];
	string lastLine = "";

	float lastz = 0;
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

	float towerRadius = 5;
	float towerPadding = 2;

	vector<string> towers;
	float centerDist = 100; //mm

	string tn;
	string ltn = "unset";
	string gc;

	bool inHop = false;

	vector<vector<float>> g1; // array of all G1's for interesection detection
	vector<float> gpos(3);
	gpos[0] = 0;
	gpos[1] = 0;
	gpos[2] = 0;


	//assign cli args to variables
	string argKey;
	string argVal;
	for(int i = 0; i < argc; i++){
		//cout << argc << i << endl;
		if(argv[i][0] != '-'){
			continue;
		}

		if(i < argc-1){
			argKey = argv[i];
			argVal = argv[i+1];
		}else{
			continue;
		}

		if(argKey == "-f" || argKey == "--file"){
			infile = argVal;
		}else if(argKey == "-o"){
			outfile = argVal;
		}else if(argKey == "--padding"){
			towerPadding = stof(argVal);
		}else if(argKey == "--radius"){
			towerRadius = stof(argVal);
		}else if(argKey == "--centerDist"){
			centerDist = stof(argVal);
		}else if(argKey == "--feedRate"){
			frate = stof(argVal);
		}else if(argKey == "--retraction"){
			retraction = stof(argVal);
		}
	}

	ifstream inf(infile);
	ofstream ouf(outfile);

	if(!inf.is_open() || !ouf.is_open()){
		cout << "error opening file" << endl;
		cout << infile << endl;
		cout << inf.is_open() << endl;
		cout << ouf.is_open() << endl;
		return 0;
	}

	float epm = getEPerMM(&inf);
	cout << "e mm per xy mm " << epm << endl;

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
			}else if(newz - zpos >= zhop - 0.01){
				//zhop here
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
	float j = 0; //iterator 
	float a = towerPadding + (2 * towerRadius);
	float b = centerDist-towerRadius-towerPadding;
	float angleInc = acos((2*b*b-a*a)/(2*b*b));
	vector<float> pos(2);
	vector<vector<float>> towerCords;
	for(int i = 0; i < maxSwaps; ){
		cout << i << endl;
		pos = p2c(angleInc*j, b);
		tdist = checkVerticalIntersect(&g1, &pos);
		cout << tdist << endl;
		if(tdist >= towerPadding+towerRadius){
			towerCords.push_back(pos);
			i++;
		}else if(angleInc*i > 2*M_PI){
			cout << "unable to place towers" << endl;
			return 0;
		}
		j++;
		//cout << tdist << endl;
	}

	string tower;
	for(int i = 0; i < maxSwaps; i++){
		float x = towerCords[i].at(0);
		float y = towerCords[i].at(1);
		tower = genTower(x, y, towerRadius, epm, frate);
		towers.push_back(tower);
	}

	//for(int i = 0; towerCords.size() < maxSwaps; i++){

	//}

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
					epos = findLastEpos(&inf);
					for(int i = swaps; i < maxSwaps; ++i){
						ouf << ";Fill Tower " << i << endl;
						drawTower(towers.at(i), &ouf, epos, inHop);
					}
					inHop = false;
				}
				swaps = 0;
				zpos = newz;
				if(inHop){
					ouf << ";Out Hop" << endl;
				}
				inHop = false;
			}else if(newz - zpos >= zhop - 0.01){
				//zhop here
				ouf << ";In Hop" << endl;
				inHop = true;
			}else if(newz == zpos){
				ouf << ";Out Hop" << endl;
				inHop = false;
			}
		}else if(gc.substr(0,1) == "T"){
			tn = gc;
			if(ltn == "unset"){
				ltn = tn;
			}else if(tn != ltn){
				ltn = tn;
				ouf << l << endl;
				if(zpos > 0){
					epos = findLastEpos(&inf);
					drawTower(towers.at(swaps), &ouf, epos, inHop);
					inHop = false;
				}
				swaps++;
			}
		}
		//if(l.find('Z') != string::npos){
		//	ouf << l << endl;
		//}else if(l.find('M') != string::npos){
		//	ouf << l << endl;
		//}
		ouf << l << endl;
	}

	cout << "done" << endl;

}
