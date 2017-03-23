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
#include <queue>
#include <pthread.h>
using namespace std;

queue<string> queue_sites;
queue<string> queue_search;
queue<string> queue_data;
vector<string> searches;
vector<int> searches_counts;
vector<string> sites;


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
  int get_fetch_threads();
  int get_parse_threads();
  void parse_input_file(string fname);
  void parse_search_file();
  void parse_site_file();
  //  void * get_site(void * args);
  //void find_words(string s);
  //void * find_words(void * args);
  int write_to_output(string num);
  void push_sites_to_queue();
  void push_search_to_queue();
 private:
  int period_fetch;
  int num_fetch;
  int num_parse;
  string search_file;
  string site_file;
};

//set default arguments
Config::Config(){
  period_fetch = 180;
  num_fetch = 1;
  num_parse = 1;
  search_file = "Search.txt";
  site_file = "Sites.txt";
}

int Config::get_fetch_threads(){
  return num_fetch;
}

int Config::get_parse_threads(){
  return num_parse;
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

void * get_site(void * args){
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
    //curl_easy_setopt(curl_handle, CURLOPT_URL, this_site.c_str());
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
      
      //push to queue_data rather than sending straight to find_words
      // MUTEX LOCK
      pthread_mutex_t mut;
      pthread_mutex_lock(&mut);
      queue_data.push(chunk.memory);
      pthread_mutex_unlock(&mut);
      // MUTEX UNLOCK
      
      //find_words(chunk.memory);
    }
    curl_easy_cleanup(curl_handle);
    //    free(chunk.memory);
    curl_global_cleanup();
  }

  // pthread_exit(NULL);    // do we need this???
  return 0;
}

void * find_words(void * args){
// void * Config::find_words(void * args){
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock(&mutex);
  string s = queue_data.back();
  queue_data.pop();
  pthread_mutex_unlock(&mutex);
  int count = 0;
  string word;
  for (size_t i = 0; i < queue_search.size(); i++) {
    count = 0;
    //    word = queue_search.at(i);
    word = searches[i];
    size_t n = s.find(word, 0);
    while(n != string::npos){
      count++;
      n = s.find(word, n+1);
    }
    //cout << count << endl;
    // write to stdout for now
    cout << count << " " << word << endl;
    searches_counts.push_back(count);
  }
  return 0;
  //write_to_output("1");
}

//not sure if there should be separate fcn for this (confused bout threading)
int Config::write_to_output(string name){
  string outfile = name + ".csv";
  ofstream outputFile(outfile.c_str());
  if (outputFile.is_open()) {
    size_t j = 0;
    for (size_t i = 0; i < searches_counts.size(); i++) {
        //cout << searches[i] <<  endl;
        outputFile << searches_counts[i] << " " << searches[j] << endl;
    	if (j == searches.size()-1) {
	    j = 0;
	}
	else {
	    j++;
	}
    }
    outputFile.close();
  }
  else {
    fprintf(stderr,"config: couldn't write to %s: %s\n",outfile.c_str(),strerror(errno));
    return 1;
  }
  return 0;
}

void Config::push_sites_to_queue() {
    for (size_t i = 0; i < sites.size(); i++) {
    	queue_sites.push(sites[i]);
    }
}

void Config::push_search_to_queue() {
    for (size_t i = 0; i < searches.size(); i++) {
        queue_search.push(searches[i]);
    }
}
