#include <iostream>
#include <string>
#include "config.h"
#include <pthread.h>
#include <sys/types.h>
#include <cstdlib>
#define MAX_THREAD 8
const int NUM_SECONDS = 10;

int main(int argc, char *argv[]){
  curl_global_init(CURL_GLOBAL_ALL);
  pthread_t *threads_f;
  pthread_t *threads_p;

  Config run;
  if (argc != 2) {
  	cout << argv[0] << ": Incorrect arguments!" << endl;
  	cout << "usage: " << argv[0] << " <configfile>" << endl;
  	exit(1);
  }
  string fname = argv[1];
  run.parse_input_file(fname);
  int num_fetch = run.get_fetch_threads();
  int num_parse = run.get_parse_threads();
  if ((num_fetch < 1) || (num_parse < 1) || (num_fetch > MAX_THREAD) || (num_parse > MAX_THREAD)) {
	printf("The number of threads should be between 1 and %d.\n", MAX_THREAD);
	exit(1);
  }
  run.parse_search_file();
  run.parse_site_file();
  run.push_search_to_queue();

  threads_f = (pthread_t *)malloc(num_fetch*sizeof(*threads_f));
  threads_p = (pthread_t *)malloc(num_parse*sizeof(*threads_p)); 
  //  pthread_mutex_t mutex;
  //  for(;;){
  while(1){
    for(int i=0; i<run.get_fetch_threads(); i++){
      cout << "creating fetch thread"<<endl;
      pthread_create(&threads_f[i], NULL, get_site_name, NULL);
    }
    for(int j=0; j<run.get_parse_threads(); j++){
      cout << "creating parse thread"<<endl;
      pthread_create(&threads_p[j], NULL, find_words, NULL);
    }
  //    for(int i=0; i < NUM_SECONDS; i++){ usleep(1000 * 1000); }
    //start timer loop
    push_sites_to_queue();
    //pthread_mutex_lock(&mutex);
    //run.push_sites_to_queue(); //populate sites queue
    //  count++;
    //pthread_cond_broadcast(&consumer_signal);
    //pthread_mutex_unlock(&mutex);
    
    sleep(5);
    //  cout << "hi"<<endl;
  }
  //free(threads_f);
  //free(threads_p);
  curl_global_cleanup();
  return 1;
}
