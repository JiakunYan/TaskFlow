#ifndef TASKFLOW_UTILS_HPP
#define TASKFLOW_UTILS_HPP

#define TF_CHECK_ABT(e) ::tf::checkABT(e, "", __FILE__, __LINE__)

namespace tf {
static inline void checkABT(int err, const char *msg, const char *file, int line)
{
  char *err_str;
  size_t len;
  int ret;

  if (err == ABT_SUCCESS)
    return;
  if (err == ABT_ERR_FEATURE_NA) {
    printf("Skipped\n");
    fflush(stdout);
    exit(77);
  }

  ret = ABT_error_get_str(err, NULL, &len);
  if (ret != ABT_SUCCESS) {
    fprintf(stderr, "%s (%d): %s (%s:%d)\n", "Unknown error", err, msg, file, line);
  } else {
    err_str = (char *)malloc(sizeof(char) * len + 1);
    assert(err_str != NULL);
    ret = ABT_error_get_str(err, err_str, NULL);

    fprintf(stderr, "%s (%d): %s (%s:%d)\n", err_str, err, msg, file, line);
    free(err_str);
  }

  exit(EXIT_FAILURE);
}
} // namespace tf

#endif // TASKFLOW_UTILS_HPP
