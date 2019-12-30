#include <rte_ethdev.h>

int hello(__attribute__((unused)) void *arg) {
  return printf("hi");
}

int port_init() {
  return printf("bye");
}

int main(int argc, char **argv) {
  unsigned lcore_id, port_id;
  rte_eal_init(argc, argv);
  RTE_ETH_FOREACH_DEV(port_id) {
    port_init();
  }
}
