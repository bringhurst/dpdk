/*-
 *   BSD LICENSE
 * 
 *   Copyright(c) 2010-2013 Intel Corporation. All rights reserved.
 *   All rights reserved.
 * 
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _VIRTIO_ETHDEV_H_
#define _VIRTIO_ETHDEV_H_

#include <stdint.h>

#include "virtio_pci.h"

#define SPEED_10	10
#define SPEED_100	100
#define SPEED_1000	1000
#define SPEED_10G	10000
#define HALF_DUPLEX	1
#define FULL_DUPLEX	2

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define VIRTIO_MAX_RX_QUEUES 1
#define VIRTIO_MAX_TX_QUEUES 1
#define VIRTIO_MAX_MAC_ADDRS 1
#define VIRTIO_MIN_RX_BUFSIZE 64
#define VIRTIO_MAX_RX_PKTLEN  1518

/* Features desired/implemented by this driver. */
#define VTNET_FEATURES \
    (VIRTIO_NET_F_MAC        | \
     VIRTIO_NET_F_STATUS     | \
     VIRTIO_NET_F_CTRL_VQ    | \
     VIRTIO_NET_F_CTRL_RX    | \
     VIRTIO_NET_F_CTRL_VLAN  | \
     VIRTIO_NET_F_CSUM       | \
     VIRTIO_NET_F_HOST_TSO4  | \
     VIRTIO_NET_F_HOST_TSO6  | \
     VIRTIO_NET_F_HOST_ECN   | \
     VIRTIO_NET_F_GUEST_CSUM | \
     VIRTIO_NET_F_GUEST_TSO4 | \
     VIRTIO_NET_F_GUEST_TSO6 | \
     VIRTIO_NET_F_GUEST_ECN  | \
     VIRTIO_NET_F_MRG_RXBUF  | \
     VIRTIO_RING_F_INDIRECT_DESC)

/*
 * RX/TX function prototypes
 */
void virtio_dev_rxtx_start(struct rte_eth_dev *dev);

int virtio_dev_queue_setup(struct rte_eth_dev *dev,
			int queue_type,
			uint16_t queue_idx,
			uint8_t  vtpci_queue_idx,
			uint16_t nb_desc,
			unsigned int socket_id,
			struct virtqueue **pvq);

int  virtio_dev_rx_queue_setup(struct rte_eth_dev *dev, uint16_t rx_queue_id,
		uint16_t nb_rx_desc, unsigned int socket_id,
		const struct rte_eth_rxconf *rx_conf,
		struct rte_mempool *mb_pool);

int  virtio_dev_tx_queue_setup(struct rte_eth_dev *dev, uint16_t tx_queue_id,
		uint16_t nb_tx_desc, unsigned int socket_id,
		const struct rte_eth_txconf *tx_conf);

uint16_t virtio_recv_pkts(void *rx_queue, struct rte_mbuf **rx_pkts,
		uint16_t nb_pkts);

uint16_t virtio_xmit_pkts(void *tx_queue, struct rte_mbuf **tx_pkts,
		uint16_t nb_pkts);

/*
 * Structure to store private data for each driver instance (for each port).
 */
struct virtio_adapter {
	struct virtio_hw hw;
};

#define VIRTIO_DEV_PRIVATE_TO_HW(adapter)\
	(&((struct virtio_adapter *)adapter)->hw)

/*
 * The VIRTIO_NET_F_GUEST_TSO[46] features permit the host to send us
 * frames larger than 1514 bytes. We do not yet support software LRO
 * via tcp_lro_rx().
 */
#define VTNET_LRO_FEATURES (VIRTIO_NET_F_GUEST_TSO4 | \
    VIRTIO_NET_F_GUEST_TSO6 | VIRTIO_NET_F_GUEST_ECN)


#endif /* _VIRTIO_ETHDEV_H_ */
