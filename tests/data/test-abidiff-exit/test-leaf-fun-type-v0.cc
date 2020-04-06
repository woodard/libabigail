struct ops {
  void (*munge)(int x);
};

void register_ops(const ops&) {
}
