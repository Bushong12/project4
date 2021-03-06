#include <iostream>
#include <string>
#include <sstream>
#include "config.h"
#include <pthread.h>
#include <sys/types.h>
#include <cstdlib>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#define MAX_THREAD 8

int keepLooping = 1;
void ourHandler(int num){
  keepLooping = 0;
}

int main(int argc, char *argv[]){
    // initialize variables
    curl_global_init(CURL_GLOBAL_ALL);
    pthread_t *threads_f;
    pthread_t *threads_p;
    Config run;
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = ourHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    // check for correct arguments
    if (argc != 2) {
        cout << argv[0] << ": Incorrect arguments!" << endl;
        cout << "usage: " << argv[0] << " <configfile>" << endl;
        exit(1);
    }

    // get info from files
    string fname = argv[1]; // config file
    run.parse_input_file(fname);    
    int num_fetch = run.get_fetch_threads();
    int num_parse = run.get_parse_threads();
    // thread count error-checking
    if ((num_fetch < 1) || (num_parse < 1) || (num_fetch > MAX_THREAD) || (num_parse > MAX_THREAD)) {
        printf("The number of threads should be between 1 and %d.\n", MAX_THREAD);
        exit(1);
    }

    //parse files
    run.parse_search_file();
    run.parse_site_file();
    run.push_search_to_queue();

    threads_f = (pthread_t *)malloc(num_fetch*sizeof(*threads_f));
    threads_p = (pthread_t *)malloc(num_parse*sizeof(*threads_p)); 
    //run every period_fetch
    while(keepLooping){
        //signal(SIGINT, ourHandler);
        //signal(SIGHUP, ourHandler);
        sigaction(SIGINT, &sigIntHandler, NULL);
        numFile++;
        stringstream ss;
        ss << numFile;
        string str = ss.str();
        initialize_output_file(str);

        //creating number of threads specified
        for(int i=0; i<run.get_fetch_threads(); i++){
            pthread_create(&threads_f[i], NULL, get_site_name, NULL);
            pthread_mutex_lock(&mutex);
            totFetch++;
            pthread_mutex_unlock(&mutex);
        }

        for(int j=0; j<run.get_parse_threads(); j++){
            pthread_create(&threads_p[j], NULL, find_words, NULL);
            pthread_mutex_lock(&mutex);
            totParse++;
            pthread_mutex_unlock(&mutex);
        }
        
        //populate sites queue
        push_sites_to_queue();
	cout << "writing to "<<numFile<<".csv"<<endl;
        sleep(run.get_period_fetch());
    }
    //graceful exit/cleanup
    while (totFetch != 0 || totParse != 0) {
        // wait for threads to finish
    }
    curl_global_cleanup();
    return 0;
}
