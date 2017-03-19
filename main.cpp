#include <iostream>
#include <string>
#include "config.h"

int main(int argc, char *argv[]){
  Config run;
  string fname = argv[1];
  run.parse_input_file(fname);
  return 1;
}
