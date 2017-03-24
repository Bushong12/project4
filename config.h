#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
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
pthread_cond_t producer_signal = PTHREAD_COND_INITIALIZER;
pthread_cond_t consumer_signal = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex;
int numFile = 0;

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
  //  int write_to_output(string num);
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

int write_to_output(string name){
  string outfile = name + ".csv";
  ofstream outputFile(outfile.c_str());
  if (outputFile.is_open()) {
    size_t j = 0;
    for (size_t i = 0; i < searches_counts.size(); i++) {
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
    return 0;
  }
  return 1;
}

void get_site(string site){
  //cout << "inside get_site" << endl;
  CURL *curl_handle;
  CURLcode res;
  
  struct MemoryStruct chunk;
  
  chunk.memory = (char*)malloc(1);
  chunk.size = 0;
  curl_handle = curl_easy_init();  
  
  curl_easy_setopt(curl_handle, CURLOPT_URL, site.c_str()); //fetch site
  curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  res = curl_easy_perform(curl_handle);
  if(res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
  }
  else{
    //producer
    pthread_mutex_lock(&mutex);
    queue_data.push(chunk.memory);
    //datacount++;
    pthread_cond_broadcast(&producer_signal);
    pthread_mutex_unlock(&mutex);
  }
  curl_easy_cleanup(curl_handle);
}

void * get_site_name(void * args){
  //consumer
  pthread_mutex_lock(&mutex);
  while(queue_sites.empty()){
    pthread_cond_wait(&consumer_signal, &mutex);
  }
  string site = queue_sites.front(); //check that this shoudl be back and not top
  queue_sites.pop();
  pthread_mutex_unlock(&mutex);
  get_site(site);
  return 0;
}

void * find_words(void * args){
  //consumer
  pthread_mutex_lock(&mutex);
  while(queue_data.empty()){
    pthread_cond_wait(&producer_signal, &mutex);
  }
  string s = queue_data.back();
  queue_data.pop();
  pthread_cond_broadcast(&consumer_signal);
  //datacount--;
  pthread_mutex_unlock(&mutex);

  int count = 0;
  string word;
  searches_counts.clear();
  for (size_t i = 0; i < queue_search.size(); i++) {
    count = 0;
    word = searches[i];
    size_t n = s.find(word, 0);
    while(n != string::npos){
      count++;
      n = s.find(word, n+1);
    }
    searches_counts.push_back(count);
    cout << "word: "<<word<<" count: "<<count<<endl;
    stringstream ss;
    ss << numFile;
    string str = ss.str();
    int result = write_to_output(str);
  }

  return 0;
  //write_to_output("1");
}


void push_sites_to_queue() {
  pthread_mutex_lock(&mutex);
  for (size_t i = 0; i < sites.size(); i++) {
    cout << "pushing site to queue" << endl;
    queue_sites.push(sites[i]);
  }
  pthread_cond_broadcast(&consumer_signal);
  pthread_mutex_unlock(&mutex);
}

void Config::push_search_to_queue() {
    for (size_t i = 0; i < searches.size(); i++) {
        queue_search.push(searches[i]);
    }
}
