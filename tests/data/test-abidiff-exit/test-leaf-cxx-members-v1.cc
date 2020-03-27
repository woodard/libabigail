struct ops {
  // TODO: type, name and size are differnent from deleted_var, but this is
  // still reported as a change rather than a deletion and an addition.
  long added_var;
  virtual long added_fn() { return 0; }

  long changed_var;
  virtual long changed_fn() { return 0; }
};

void reg(ops& x) {
  ops instantiate = x;
  (void) instantiate;
}
