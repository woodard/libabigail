struct foo {
  long z;
};

struct ops1 {
  int ** x;
};

struct ops2 {
  // A change to foo's size is currently considered local here.  Arguably this
  // should be considered non-local as the change to foo is being reported
  // independently.  If this happens, the test case will need to be updated (to
  // remove the reporting of an ops5 diff).
  foo y[10];
};

struct ops3 {
  void (*spong)(int && wibble);
};

struct ops4 {
  int & x;
};

struct ops5 {
  int *** x;
};

// TODO: This *should* be considered a local change, but currently is not.
int var6[5][2];

void register_ops1(ops1*) { }
void register_ops2(ops2*) { }
void register_ops3(ops3*) { }
void register_ops4(ops4*) { }
void register_ops5(ops5*) { }
