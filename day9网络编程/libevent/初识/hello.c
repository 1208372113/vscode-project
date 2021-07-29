#include <event2/event.h>
#include <stdio.h>

int main(int argc, char const* argv[]) {
  const char** p = event_get_supported_methods();
  int i = 0;
  while (p[i] != NULL) {
    printf("%s\n", p[i++]);
  }
  //获取地基节点
  struct event_base* base = event_base_new();
  if (base == NULL) {
    perror("event_base_new error");
    return -1;
  }

  //查看当前系统使用方法
  const char* pp = event_base_get_method(base);
  printf("%s\n", pp);

  //释放地基节点
  event_base_free(base);

  return 0;
}
