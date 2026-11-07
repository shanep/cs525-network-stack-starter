#include "virtio_net.h"


bool virtio_net_init(virtio_net_t *nic, const pci_device_t *dev) {
  (void)nic; (void)dev; return false;
}

void virtio_net_mac(const virtio_net_t *nic, uint8_t mac[6]) {
  (void)nic; (void)mac;
}

netif_t virtio_net_netif(virtio_net_t *nic) {
  netif_t netif = {0}; (void)nic; return netif;
}
