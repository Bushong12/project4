#include <iostream>
#include <string>
#include "config.h"

int main(int argc, char *argv[]){
  Config run;
  if (argc != 2) {
  	cout << argv[0] << ": Incorrect arguments!" << endl;
  	cout << "usage: " << argv[0] << " <configfile>" << endl;
  	exit(1);
  }
  string fname = argv[1];
  run.parse_input_file(fname);
  run.parse_search_file();
  run.parse_site_file();
  run.get_site();
  return 1;
}
