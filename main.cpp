#include <iostream>
#include <string>
#include "config.h"
#include <pthread>
#include <sys/types.h>

#define MAX_THREAD 1000

int main(int argc, char *argv[]){
  pthread_t *threads;
  thread_args_struct *t;

  Config run;
  if (argc != 2) {
  	cout << argv[0] << ": Incorrect arguments!" << endl;
  	cout << "usage: " << argv[0] << " <configfile>" << endl;
  	exit(1);
  }
  string fname = argv[1];
  run.parse_input_file(fname);
  if ((run.get_fetch_threads() < 1) || (run.get_parse_threads() < 1) || (run.get_fetch_threads() > MAX_THREAD) || (run.get_parse_threads() > MAX_THREAD)) {
	printf("The number of threads should be between 1 and %d.\n", MAX_THREAD);
	exit(1);
  }
  run.parse_search_file();
  run.parse_site_file();
  run.get_site();
  return 1;
}
