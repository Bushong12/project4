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

//global variables
queue<string> queue_sites;
queue<string> queue_search;
queue<string> queue_data;
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
        int get_period_fetch();
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

void Config::push_search_to_queue() {
    for (size_t i = 0; i < searches.size(); i++) {
        queue_search.push(searches[i]);
    }
}

void write_to_output(string name){
    pthread_mutex_lock(&mutex);
    while(queue_word_counts.empty()){ // nothing to write
        pthread_cond_wait(&third_signal, &mutex);
    }

    cout << "writing to output"<<endl;

    //int limit = sites.size() * searches.size(); // for loop: total site/search combos 
    int limit = searches.size();
    string outfile = name + ".csv";             // set file name for this iteration
    ofstream outputFile;       // initialize file
    outputFile.open(outfile.c_str(), ios_base::app);    // open the file to append to it
    if (outputFile.is_open()) {
        //size_t j = 0;
        //outputFile << queue_word_counts.back().first << " "<<queue_word_counts.back().second<<endl;
        //queue_word_counts.pop();
        for (int i = 0; i < limit; i++) {         // for each site/ word combo
            //not writing to file
            string tmpWord = queue_word_counts.front().first; 
            int tmpCount = queue_word_counts.front().second;
            queue_word_counts.pop(); // THIS LINE IS CAUSING SEGFAULT // pops the front
            outputFile << tmpWord<<" "<<tmpCount <<endl;
            /*outputFile << searches_counts[i] << " " << searches[j] << endl;
              if (j == searches.size()-1) {
              j = 0;
              }
              else {
              j++;
              }*/
        }
        outputFile.close();
    }
    else {
        fprintf(stderr,"config: couldn't write to %s: %s\n",outfile.c_str(),strerror(errno));
    }
    pthread_mutex_unlock(&mutex);
}

/*void initialize_output(string name) {
    pthread_mutex_lock(&mutex);
    ofstream outputF*/


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
    cout << "inside get_site_name"<<endl;
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
    cout << "inside find_words"<<endl;
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
    //searches_counts.clear();
    //
    pthread_mutex_lock(&mutex); //LOCK: might not need?
    queue_word_counts.empty();
    cout << "new site: " << endl;
    
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
        //searches_counts.push_back(count);
        queue_word_counts.push(make_pair(word, count));
        cout << "word: "<<word<<" count: "<<count<<endl;
        //cout << queue_word_counts.back().first << " "<<queue_word_counts.back().second<<endl;
    }

    pthread_cond_broadcast(&third_signal);
    pthread_mutex_unlock(&mutex); //UNLOCK
    write_to_output(str);

    return 0;
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
