#include <iostream>
#include <rte_ethdev.h>
#include <rte_mbuf.h>

constexpr uint16_t BURST_SIZE = 32;

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

[[noreturn]] static void lcore_main(void)
{
  while(true) {
    rte_mbuf *packets[BURST_SIZE];
    const uint16_t nb_pkts_rx = rte_eth_rx_burst(0, 0, packets, BURST_SIZE);
    const uint16_t nb_pkts_tx = rte_eth_tx_burst(1, 0, packets, BURST_SIZE);
    for (uint16_t i = nb_pkts_tx; i < nb_pkts_rx; i++) {
      // rte_pktmbuf_free(packets[i]);
    }
  }
}

int main(int argc, char **argv) {
  unsigned lcore_id, port_id;
  //rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
  //  "mbuf_pool", 7, 250, 0, RTE_MBUF_DEFAULT_BUF_SIZE, SOCKET_ID_ANY);
  rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * 2,
    MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
  std::cout << rte_strerror(rte_errno) << std::endl;
  rte_eth_conf port_conf {};
  rte_eal_init(argc, argv);
  RTE_ETH_FOREACH_DEV(port_id) {
    rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    rte_eth_rx_queue_setup(port_id, 0, 1024, SOCKET_ID_ANY, NULL, mbuf_pool);
    rte_eth_tx_queue_setup(port_id, 0, 1024, SOCKET_ID_ANY, NULL);
    rte_eth_dev_start(port_id);
    rte_eth_promiscuous_enable(port_id);
  }
}
