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
  Config();
  void parse_input_file(string fname);
  void parse_search_file();
  void parse_site_file();
 private:
  int period_fetch;
  int num_fetch;
  int num_parse;
  string search_file;
  string site_file;
  vector<string> searches;
  vector<string> sites;
};

//set default arguments
Config::Config(){
  period_fetch = 180;
  num_fetch = 1;
  num_parse = 1;
  search_file = "Search.txt";
  site_file = "Sites.txt";
}

void Config::parse_input_file(string fname){
  ifstream infile(fname.c_str());
  string line;
  while(getline(infile, line)){
    size_t found = line.find("=");
    string param, arg;
    for(int i = 0; i < found; i++){
      param = param + line[i];
    }
    for(int i = found + 1; i < line.size(); i++){
      arg = arg + line[i];
    }
    if(param == "PERIOD_FETCH")
      period_fetch = atoi(arg.c_str());
    else if(param == "NUM_FETCH")
      num_fetch = atoi(arg.c_str());
    else if(param == "NUM_PARSE")
      num_parse = atoi(arg.c_str());
    else if(param == "SEARCH_FILE")
      search_file = arg;
    else if(param == "SITE_FILE")
      site_file = arg;
  }
}

void Config::parse_search_file(){
  ifstream infile(search_file.c_str());
  string line;
  while(getline(infile, line)){
    searches.push_back(line);
  }
}

void Config::parse_site_file(){
  ifstream infile(site_file.c_str());
  string line;
  while(getline(infile, line)){
    sites.push_back(line);
  }
}
