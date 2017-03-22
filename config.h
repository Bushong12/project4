#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include <errno.h>
using namespace std;

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp){
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

class Config {
 public:
  Config();
  void parse_input_file(string fname);
  void parse_search_file();
  void parse_site_file();
  void get_site();
  void find_words(string s);
  int write_to_output(string num);
 private:
  int period_fetch;
  int num_fetch;
  int num_parse;
  string search_file;
  string site_file;
  vector<string> searches;
  vector<int> searches_counts;
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
    for(size_t i = 0; i < found; i++){
      param = param + line[i];
    }
    for(size_t i = found + 1; i < line.size(); i++){
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

void Config::get_site(){
  for (size_t i = 0; i < sites.size(); i++) {
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;
    //cout << sites[i] << endl;

    chunk.memory = (char*)malloc(1);
    chunk.size = 0;
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    //NOTE - don't hardcode url
    curl_easy_setopt(curl_handle, CURLOPT_URL, sites[i].c_str());
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    res = curl_easy_perform(curl_handle);
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    else{
      //    printf("%s", chunk.memory);
      //    printf("%lu bytes retrieved\n", (long)chunk.size);
      find_words(chunk.memory);
    }
    curl_easy_cleanup(curl_handle);
    free(chunk.memory);
    curl_global_cleanup();
  }
}

void Config::find_words(string s){
  int count = 0;
  string word;
  for (size_t i = 0; i < searches.size(); i++) {
    count = 0;
    word = searches[i];
    size_t n = s.find(word, 0);
    while(n != string::npos){
      count++;
      n = s.find(word, n+1);
    }
    searches_counts.push_back(count);
  }
  write_to_output("1");
}

//not sure if there should be separate fcn for this (confused bout threading)
int Config::write_to_output(string name){
  string outfile = name + ".csv";
  ofstream outputFile(outfile.c_str());
  if (outputFile.is_open()) {
    for (size_t i = 0; i < searches_counts.size(); i++) {
      outputFile << searches_counts[i] << " " << searches[i] << endl;
    }
    outputFile.close();
  }
  else {
    fprintf(stderr,"config: couldn't write to %s: %s\n",outfile.c_str(),strerror(errno));
    return 1;
  }
  return 0;
}
