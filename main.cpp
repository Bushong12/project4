#include <iostream>
#include <string>
#include "config.h"

int main(int argc, char *argv[]){
  Config run;
  string fname = argv[1];
  run.parse_input_file(fname);
  run.parse_search_file();
  run.parse_site_file();
  return 1;
}
