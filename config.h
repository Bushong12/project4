#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

class Config {
 public:
  void parse_input_file(string fname);
  void parse_search_file(string fname);
  void parse_site_file(string fname);
 private:
  int period_fetch;
  int num_fetch;
  int num_parse;
  string search_file;
  string site_file;
};

void Config::parse_input_file(string fname){
  ifstream infile(fname.c_str());
  string line;
  while(getline(infile, line)){
    string word1 = '\0';
    string word2 = '\0';
    string param, arg;
    for(int i=0; i<line.length(); i++){
      if(line[i] == '='){
	param = word1;
	for(int j=i; j < line.length(); j++){
	  word2 = word2 + line[j];
	}
	arg = word2;
	if(param == "PERIOD_FETCH"){
	  period_fetch = atoi(arg.c_str());
	  cout << "period fetch" << endl;
	}
	/*switch(param){
	case "PERIOD_FETCH":
	  cout << "DOPE" << endl;
	  break;
	default:
	  cout << "no" << endl;
	  }*/
	return;
      }
      else{
	word1 = word1 + line[i];
      }
    }
    /*char input[1000] = line.c_str();
    char *input[] = {line};
    char *token = strtok(input, "=");
    while(token != NULL){
      cout << token << " ";
      token = strtok(NULL, " ");
    }
    cout << endl;*/
  }
}
