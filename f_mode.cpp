#include <rte_ethdev.h>
#include <rte_mbuf.h>

constexpr auto BURST_SIZE = 32;

constexpr auto MBUF_CACHE_SIZE = 256;

/* The optimum size (in terms of memory usage) for a mempool
 * is when n is a power of two minus one:
 * n = (2^q - 1) */
constexpr auto NUM_BUFS = 8191;

/* Forward packets between NICs */
[[noreturn]] static void forward(void) {
  uint16_t port_id;
  rte_mbuf *packets[BURST_SIZE];
  while(true) {
    RTE_ETH_FOREACH_DEV(port_id) {
      const auto nb_pkts_rx = rte_eth_rx_burst(port_id, 0, packets, BURST_SIZE);
      /* forward received packets to paired ethernet device */
      const auto nb_pkts_tx = rte_eth_tx_burst(port_id ^ 1, 0, packets, nb_pkts_rx);
      for (auto i = nb_pkts_tx; i < nb_pkts_rx; i++) {
        rte_pktmbuf_free(packets[i]);
      }
    }
  }
}

int main(int argc, char **argv) {
  uint16_t port_id;
  struct rte_ether_addr addr;
  rte_eth_conf port_conf {};
  
  rte_eal_init(argc, argv);
  rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
    "MBUF_POOL", NUM_BUFS, MBUF_CACHE_SIZE,
    0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
  
  RTE_ETH_FOREACH_DEV(port_id) {
    rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    rte_eth_rx_queue_setup(port_id, 0, 1024, SOCKET_ID_ANY, NULL, mbuf_pool);
	  rte_eth_macaddr_get(port_id, &addr);
    RTE_LOG(INFO, EAL,
        "Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
                    " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
        port_id,
        addr.addr_bytes[0], addr.addr_bytes[1],
        addr.addr_bytes[2], addr.addr_bytes[3],
        addr.addr_bytes[4], addr.addr_bytes[5]);
    rte_eth_tx_queue_setup(port_id, 0, 1024, SOCKET_ID_ANY, NULL);
    rte_eth_dev_start(port_id);
    rte_eth_promiscuous_enable(port_id);
  }

  forward();
}
