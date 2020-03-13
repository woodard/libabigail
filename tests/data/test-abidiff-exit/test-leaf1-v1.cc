struct changed {
  long foo = 0;
};

long directly_changed_var = 0;
changed * indirectly_changed_var;
long added_var = 0;

long directly_changed_fun() {
  return 0;
}
void indirectly_changed_fun(changed * x) {
}
long added_fun() {
  return 0;
}
