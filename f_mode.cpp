#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <utility>
#include <vector>

void reply_to_arp_request(
  const rte_ether_addr rx_port_mac_addr,
  rte_ether_hdr *ether_hdr,
  rte_mbuf *packet)
{
  ether_hdr->d_addr = ether_hdr->s_addr;
  ether_hdr->s_addr = rx_port_mac_addr;
  auto arp_hdr = rte_pktmbuf_mtod_offset(
    packet, rte_arp_hdr *, RTE_ETHER_HDR_LEN);
  arp_hdr->arp_opcode = rte_cpu_to_be_16(RTE_ARP_OP_REPLY);
  arp_hdr->arp_data.arp_tha = rx_port_mac_addr;
}

void reply_to_icmp_ping(
  rte_ether_hdr *ether_hdr,
  rte_mbuf *packet)
{
  std::swap(ether_hdr->s_addr, ether_hdr->d_addr);
  auto ipv4_hdr = rte_pktmbuf_mtod_offset(
    packet, rte_ipv4_hdr *, RTE_ETHER_HDR_LEN);
  std::swap(
    (rte_be32_t &)ipv4_hdr->src_addr,
    (rte_be32_t &)ipv4_hdr->dst_addr);
  auto icmp_hdr = rte_pktmbuf_mtod_offset(packet, rte_icmp_hdr *,
    RTE_ETHER_HDR_LEN +
    (ipv4_hdr->version_ihl & RTE_IPV4_HDR_IHL_MASK) * RTE_IPV4_IHL_MULTIPLIER);
  icmp_hdr->icmp_type = RTE_IP_ICMP_ECHO_REPLY;
}

[[noreturn]] static void packet_processing_loop(
  std::vector<uint16_t> port_id_cache,
  rte_ether_addr *port_macs)
{
  uint16_t port_id;
  constexpr auto BURST_SIZE = 32;
  rte_mbuf *packets[BURST_SIZE];

  while(true) {
    for (auto port_id : port_id_cache) {
      /* read burst of packets */
      const auto nb_pkts_rx = rte_eth_rx_burst(port_id, 0, packets, BURST_SIZE);
      for (auto i = 0; i < nb_pkts_rx; i++) {
        /* extract information from ethernet header */
        auto ether_hdr = rte_pktmbuf_mtod(packets[i], rte_ether_hdr *);
        auto ether_type = rte_be_to_cpu_16(ether_hdr->ether_type);
        if (ether_type == RTE_ETHER_TYPE_ARP) {
          reply_to_arp_request(port_macs[port_id], ether_hdr, packets[i]);
        } else if (ether_type == RTE_ETHER_TYPE_IPV4) {
          reply_to_icmp_ping(ether_hdr, packets[i]);
        } else {
          printf("Received an unhandled packet with ethertype 0x%04x\n", ether_type);
        }
      }
      /* send transformed packets back on the same port */
      const auto nb_pkts_tx = rte_eth_tx_burst(port_id, 0, packets, nb_pkts_rx);
      /* free any unsent packets */
      for (auto i = nb_pkts_tx; i < nb_pkts_rx; i++) {
        rte_pktmbuf_free(packets[i]);
      }
    }
  }
}

int main(int argc, char **argv) {
  constexpr auto MBUF_CACHE_SIZE = 256;
  constexpr auto NUM_BUFS = 8191;
  constexpr auto NUM_DESCRIPTORS = 1024;
  constexpr auto NUM_QUEUES = 1;
  uint16_t port_id;
  /* Needed port caching to avoid scanning for ethernet drivers in the tight packet loop
   * See flamegraphs in the perf folder */
  std::vector<uint16_t> port_id_cache;

  rte_ether_addr port_macs[rte_eth_dev_count_avail()];
  rte_eth_conf eth_conf {};

  rte_eal_init(argc, argv);
  rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
    "MBUF_POOL", NUM_BUFS, MBUF_CACHE_SIZE,
    0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

  RTE_ETH_FOREACH_DEV(port_id) {
    port_id_cache.push_back(port_id);
    rte_eth_dev_configure(port_id, NUM_QUEUES, NUM_QUEUES, &eth_conf);
    rte_eth_rx_queue_setup(port_id, 0, NUM_DESCRIPTORS, SOCKET_ID_ANY, NULL, mbuf_pool);
    rte_eth_macaddr_get(port_id, &port_macs[port_id]);
    rte_eth_tx_queue_setup(port_id, 0, NUM_DESCRIPTORS, SOCKET_ID_ANY, NULL);
    rte_eth_dev_start(port_id);
  }

  packet_processing_loop(port_id_cache, port_macs);
}
