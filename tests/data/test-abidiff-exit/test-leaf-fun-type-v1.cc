struct ops {
  char (*munge)(long x, bool gunk);
};

void register_ops(const ops&) {
}
