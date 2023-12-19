#pragma once
struct Range {
 public:
  Range(int min = 0, int max = 0) : min{min}, max{max} {}
  int min;
  int max;
};
