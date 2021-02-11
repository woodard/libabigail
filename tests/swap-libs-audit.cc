#include <link.h>
  
#include <stdlib.h>

#include <string>
#include <fstream>
#include <map>

using std::string;
using std::ifstream;
using std::map;
using std::pair;

bool loaded=false;
bool failed=false;
map<string, string> swaps;

static void load_swapfile(){
  char *swap_filename=getenv("SWAP_LIB_FILENAME");
  if (swap_filename == NULL)
    {
      failed=true;
      return;
    }
  
  ifstream swapfile;
  try {
    swapfile.open(swap_filename);
    while (!swapfile.eof()){
      string lib;
      string replacement;
      swapfile >> lib >> replacement;
      swaps.insert( make_pair( lib, replacement));
    }
    swapfile.close();
  }
  catch (std::ifstream::failure e) {
    failed=true;
    return;
  }
}

extern "C" {

unsigned int la_version( unsigned int v){
  return LAV_CURRENT;
}

char *la_objsearch(const char *name, uintptr_t *cookie, unsigned int flag){
  if (failed)
    return NULL;
  if (!loaded)
    load_swapfile();
  if ( swaps.find(name) != swaps.end())
    return const_cast<char*>(swaps[name].c_str());
  // it is noted that that name parameter is a const char* while the return
  // value is char *. It is a bug in the interface.
  return const_cast<char*>(name);
}

}
