/*
 * A polled driver for QEMU's legacy virtio-net card. The fixed interface is every
 * function below. The virtio_net_t struct is yours.
 */
#ifndef VIRTIO_NET_H
#define VIRTIO_NET_H

#include "kernel.h"
#include "netif.h"

#define VIRTIO_VENDOR 0x1AF4
#define VIRTIO_NET_DEVICE 0x1000


/** @brief The driver's state. Its rings and buffers are big, so make it static. */
typedef struct {
  uint16_t io; uint8_t mac[6]; // and everything else you need
} virtio_net_t;

/**
 * @brief Reset the card, negotiate features, set up the rings and start it.
 *
 * Interrupts stay off the whole time, so the driver finds out about finished
 * work by looking at the used rings when it is called.
 *
 * @param nic The state to fill in.
 * @param dev The card, from pci_find().
 * @return false if the card is not one we can drive.
 */
bool virtio_net_init(virtio_net_t *nic, const pci_device_t *dev);

/** @brief Copy out the card's hardware address. */
void virtio_net_mac(const virtio_net_t *nic, uint8_t mac[6]);

/** @brief Wrap the driver up as the netif_t the stack expects. */
netif_t virtio_net_netif(virtio_net_t *nic);

#endif // VIRTIO_NET_H
