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
#include <ctime>
using namespace std;

//global variables
queue<string> queue_sites;
queue<string> queue_search;
queue<pair<string, string> > queue_data; //stores website name and its data
queue<pair<string, int> > queue_word_counts;    // stores the search word and the count on a site
vector<string> searches;
vector<int> searches_counts;
vector<string> sites;
pthread_cond_t producer_signal = PTHREAD_COND_INITIALIZER;
pthread_cond_t consumer_signal = PTHREAD_COND_INITIALIZER;
pthread_cond_t third_signal = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex;
int numFile = 0;

struct MemoryStruct {
    char *memory;
    size_t size;
};

//curls website - taken from libcurl documentation
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

//class to parse files
class Config {
    public:
        Config();
        int get_fetch_threads();
        int get_parse_threads();
        int get_period_fetch();
        void parse_input_file(string fname);
        void parse_search_file();
        void parse_site_file();
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

int Config::get_period_fetch(){
    return period_fetch;
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

//populate search queue for find_words function
void Config::push_search_to_queue() {
    for (size_t i = 0; i < searches.size(); i++) {
        queue_search.push(searches[i]);
    }
}

//writes to CSV files using condition variables/thread safety
void write_to_output(string name, string sitename){
    pthread_mutex_lock(&mutex);
    while(queue_word_counts.empty()){ // nothing to write
        pthread_cond_wait(&third_signal, &mutex);
    }

    //split string into site name and date
    string delim = ",";
    size_t pos = 0;
    string site;
    while((pos = sitename.find(delim)) != string::npos){
      site = sitename.substr(0, pos);
      sitename.erase(0, pos + delim.length());
    }
    string date = sitename;

    int limit = searches.size();
    string outfile = name + ".csv"; // set file name for this iteration
    ofstream outputFile;       // initialize file
    outputFile.open(outfile.c_str(), ios_base::app);    // open the file to append to it
    if (outputFile.is_open()) {
        for (int i = 0; i < limit; i++) {         // for each site/ word combo
            string tmpWord = queue_word_counts.front().first; 
            int tmpCount = queue_word_counts.front().second;
            queue_word_counts.pop();
            outputFile << date<< ","<<tmpWord<<"," <<site << "," << tmpCount <<endl;
        }
        outputFile.close();
    }
    else {
        fprintf(stderr,"config: couldn't write to %s: %s\n",outfile.c_str(),strerror(errno));
    }
    pthread_mutex_unlock(&mutex);
}

//add header line to output file
void initialize_output_file(string name) {
    pthread_mutex_lock(&mutex);
    string outfile = name + ".csv";
    ofstream outputFile;
    outputFile.open(outfile.c_str(), ios_base::trunc);  // erase contents in file before this
    if (outputFile.is_open()) {
        outputFile << "Time,Phrase,Site,Count" << endl;
        outputFile.close();
    }
    else {
        fprintf(stderr, "config: couldn't write to %s: %s\n", outfile.c_str(), strerror(errno));
    }
    pthread_mutex_unlock(&mutex);
}

//get current time - used in get_site
string get_time() {
    time_t t = time(0); // get time now
    struct tm * now = localtime( & t );
    stringstream dt;
    dt   << (now->tm_year + 1900) << '-'
         << (now->tm_mon + 1) << '-'
         << now->tm_mday << " "
         << (now->tm_hour + 1) << ':'
         << (now->tm_min + 1) << ':'
         << (now-> tm_sec + 1);
    string datetime = dt.str();
    return datetime;
}

void get_site(string site){
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
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 30);
    res = curl_easy_perform(curl_handle);
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    else{
        //producer
        pthread_mutex_lock(&mutex);
        string csv_info = site + "," + get_time();
	    //queue_data contains site name, time, and website data
	    queue_data.push(make_pair(csv_info, chunk.memory));
        pthread_cond_broadcast(&producer_signal); //signal to find_words
        pthread_mutex_unlock(&mutex);
    }
    curl_easy_cleanup(curl_handle);
}

void * get_site_name(void * args){
    //consumer
    pthread_mutex_lock(&mutex);
    while(queue_sites.empty()){ //no sites to search yet
        pthread_cond_wait(&consumer_signal, &mutex);
    }
    string site = queue_sites.front();
    queue_sites.pop();
    pthread_mutex_unlock(&mutex);
    get_site(site);
    return 0;
}

void * find_words(void * args){
    //consumer
    pthread_mutex_lock(&mutex);
    while(queue_data.empty()){ //no data to parse yet
        pthread_cond_wait(&producer_signal, &mutex);
    }
    string s = queue_data.back().second;
    string sitename = queue_data.back().first;
    queue_data.pop();
    pthread_cond_broadcast(&consumer_signal);
    pthread_mutex_unlock(&mutex);

    int count = 0;
    string word;
    
    pthread_mutex_lock(&mutex);
    queue_word_counts.empty();
    
    // get file name
    stringstream ss;
    ss << numFile;
    string str = ss.str();

    // for each word in search queue, store the number of occurrences
    for (size_t i = 0; i < queue_search.size(); i++) {
        count = 0;
        word = searches[i];
        size_t n = s.find(word, 0);
        while(n != string::npos){
            count++;
            n = s.find(word, n+1);
        }
        queue_word_counts.push(make_pair(word, count));
    }

    pthread_cond_broadcast(&third_signal); //broadcast to CV in write_to_output
    pthread_mutex_unlock(&mutex);
    write_to_output(str, sitename);

    return 0;
}


void push_sites_to_queue() {
    pthread_mutex_lock(&mutex);
    for (size_t i = 0; i < sites.size(); i++) {
        queue_sites.push(sites[i]);
    }
    pthread_cond_broadcast(&consumer_signal);
    pthread_mutex_unlock(&mutex);
}
