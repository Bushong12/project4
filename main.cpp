#include <iostream>
#include <string>
#include "config.h"
#include <pthread.h>
#include <sys/types.h>

#define MAX_THREAD 1000

int main(int argc, char *argv[]){
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
  threads_f = (pthread_t *)malloc(num_fetch*sizeof(*threads_f));
  threads_p = (pthread_t *)malloc(num_parse*sizeof(*threads_p)); 
 
  run.parse_search_file();
  run.parse_site_file();
  run.get_site();
  return 1;
}
