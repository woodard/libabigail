struct stn1 {
  int x;
};

struct stn2 {
  long x;
  double y;
};

void fn1(struct stn1 s) {
}

void fn2(struct stn2 s) {
}

void fn3(struct stn1* s) {
}

void fn4(struct stn2* s) {
}
