
#include <unistd.h>

#if ! defined(ARDUINO)
extern void loop(void);
extern void setup(void);

int
main(void)
{
  setup();
  while (1) {
    loop();
    usleep(30000);
  }
  return 0;
}

#endif

