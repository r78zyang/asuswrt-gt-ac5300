/*
   <:copyright-BRCM:2013:DUAL/GPL:standard

   Copyright (c) 2013 Broadcom 
   All Rights Reserved

   Unless you and Broadcom execute a separate written software license
   agreement governing use of this software, this software is licensed
   to you under the terms of the GNU General Public License version 2
   (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
   with the following added to such license:

   As a special exception, the copyright holders of this software give
   you permission to link this software with independent modules, and
   to copy and distribute the resulting executable under terms of your
   choice, provided that you also meet, for each linked independent
   module, the terms and conditions of the license of that module.
   An independent module is a module which is not derived from this
   software.  The special exception does not apply to any modifications
   of the software.

   Not withstanding the above, under no circumstances may you combine
   this software in any way with any other Broadcom software provided
   under a license other than the GPL, without Broadcom's express prior
   written consent.

   :>
 */

/******************************************************************************/
/*                                                                            */
/* File Description:                                                          */
/*                                                                            */
/* This file contains the implementation of the Runner CPU ring interface     */
/*                                                                            */
/******************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* Include files                                                             */
/*                                                                           */
/*****************************************************************************/
#ifndef RDP_SIM
#include "rdp_cpu_ring.h"
#ifdef XRDP
#include "rdd_cpu_rx.h"
#include "rdp_drv_proj_cntr.h"
#endif
#include "rdp_cpu_ring_inline.h"
#include "rdp_mm.h"

#ifndef _CFE_
#ifndef XRDP
#include "rdpa_cpu_helper.h"
#else
#include "rdpa_port_int.h"
#include "bdmf_system.h"
#endif
#endif

#if defined(__KERNEL__)
static uint32_t		init_shell = 0;
int stats_reason[2][rdpa_cpu_reason__num_of]  = {}; /* reason statistics for US/DS */
EXPORT_SYMBOL(stats_reason);
#endif

#ifdef __KERNEL__
#define shell_print(priv,format, ...) bdmf_session_print((bdmf_session_handle)priv,format,##__VA_ARGS__)
#elif defined(_CFE_)
#define shell_print(dummy,format,...) xprintf(format, ##__VA_ARGS__)
#endif

#if !defined(__KERNEL__) && !defined(_CFE_)
#error "rdp_cpu_ring is supported only in CFE and Kernel modules"
#endif

RING_DESCTIPTOR host_ring[D_NUM_OF_RING_DESCRIPTORS] = {};
EXPORT_SYMBOL(host_ring);

int cpu_ring_shell_list_rings(void *shell_priv, int start_from)
{
    uint32_t cntr;
    uint32_t first = 0, last = D_NUM_OF_RING_DESCRIPTORS;
#ifdef XRDP
    uint32_t read_idx = 0, write_idx = 0;
#endif

    shell_print(shell_priv, "CPU RX Ring Descriptors \n" );
    shell_print(shell_priv, "------------------------------\n" );

    if (start_from != -1)
    {
        first = start_from;
        last = first + 1;
    }

    for (cntr = first; cntr < last; cntr++)
    {
        char *ring_type;
        if (!host_ring[cntr].num_of_entries) 
            continue;

        ring_type = "RX";
#ifdef XRDP
        rdp_cpu_get_read_idx(host_ring[cntr].ring_id, host_ring[cntr].type, &read_idx);
        rdd_cpu_get_write_idx(host_ring[cntr].ring_id, host_ring[cntr].type, &write_idx);

        if (host_ring[cntr].type == rdpa_ring_feed)
            ring_type = "Feed";
        else  if (host_ring[cntr].type == rdpa_ring_recycle)
            ring_type = "Recycle";
#endif

        shell_print(shell_priv, "CPU %s Ring Queue = %d:\n", ring_type, cntr );
        shell_print(shell_priv, "\tCPU %s Ring Queue id= %d\n",ring_type, host_ring[cntr].ring_id );
        shell_print(shell_priv, "\tNumber of entries = %d\n", host_ring[cntr].num_of_entries );
        shell_print(shell_priv, "\tSize of entry = %d bytes\n", host_ring[cntr].size_of_entry );
        shell_print(shell_priv, "\tAllocated Packet size = %d bytes\n", host_ring[cntr].packet_size );
        shell_print(shell_priv, "\tRing Base address = 0x%pK\n", host_ring[cntr].base );
#ifndef XRDP
        shell_print(shell_priv, "\tRing Head address = 0x%pK\n", host_ring[cntr].head );
        shell_print(shell_priv, "\tRing Head position = %ld\n",
            (long)(host_ring[cntr].head - (CPU_RX_DESCRIPTOR *)host_ring[cntr].base));
#else
        shell_print(shell_priv, "\tRing Write index = %d shadow = %d\n", write_idx, host_ring[cntr].shadow_write_idx);
        shell_print(shell_priv, "\tRing Read index = %d shadow = %d\n", read_idx, host_ring[cntr].shadow_read_idx);
#endif
        shell_print(shell_priv, "\tCurrently Queued = %d\n", rdp_cpu_ring_get_queued(cntr));
        shell_print(shell_priv, "-------------------------------\n" );
        shell_print(shell_priv, "\n\n" );
    }

    return 0;
}

/* XXX: Separate RDP/XRDP implementation, move XRDP to 6858 */
#ifdef XRDP
static void cpu_ring_pd_print_fields(void *shell_priv, CPU_RX_DESCRIPTOR* pdPtr)
{
    shell_print(shell_priv, "descriptor fields:\n");
    if (pdPtr->abs.abs)
    {
        shell_print(shell_priv, "\ttype: absolute address\n");
        shell_print(shell_priv, "\tpacket DDR uncached address low: 0x%pK\n",(void*)PHYS_TO_UNCACHED(pdPtr->abs.host_buffer_data_ptr_low));
        shell_print(shell_priv, "\tpacket DDR uncached address hi: 0x%pK\n",(void*)PHYS_TO_UNCACHED(pdPtr->abs.host_buffer_data_ptr_hi));
        shell_print(shell_priv, "\tpacket len: %d\n", pdPtr->abs.packet_length);
    }
    else
    {
        shell_print(shell_priv, "\ttype: fpm\n");
        shell_print(shell_priv, "\tfpm_id: 0x%x\n", pdPtr->fpm.fpm_idx);
        shell_print(shell_priv, "\tpacket len: %d\n", pdPtr->fpm.packet_length);
    }

    if (pdPtr->cpu_vport.vport >= RDD_CPU_VPORT_FIRST && pdPtr->cpu_vport.vport <= RDD_CPU_VPORT_LAST)
    {
        shell_print(shell_priv, "\tsource: WLAN\n");
        shell_print(shell_priv, "\tdata offset: %d\n", pdPtr->cpu_vport.data_offset);
        shell_print(shell_priv, "\treason: %d\n", pdPtr->cpu_vport.reason);
        shell_print(shell_priv, "\tssid: %d\n", pdPtr->cpu_vport.ssid);
        shell_print(shell_priv, "\tvport: %d\n", pdPtr->cpu_vport.vport);

        shell_print(shell_priv, "\tis_rx_offload: %d\n", pdPtr->is_rx_offload);
        shell_print(shell_priv, "\tis_ucast: %d\n", pdPtr->is_ucast);
        shell_print(shell_priv, "\ttx_prio: %d\n", pdPtr->wl_nic.tx_prio);
        shell_print(shell_priv, "\tdst_ssid_vector / metadata: 0x%x\n", pdPtr->dst_ssid_vector);
        shell_print(shell_priv, "\twl_metadata: 0x%x\n", pdPtr->wl_metadata);
    }
    else if (pdPtr->wan.is_src_lan)
    {
        shell_print(shell_priv, "\tsource: LAN\n");
        shell_print(shell_priv, "\tdata offset: %d\n", pdPtr->lan.data_offset);
        shell_print(shell_priv, "\treason: %d\n", pdPtr->lan.reason);
        shell_print(shell_priv, "\tsource port: %d\n", pdPtr->lan.source_port);
    }
    else
    {
        shell_print(shell_priv, "\tsource: WAN\n");
        shell_print(shell_priv, "\tdata offset: %d\n", pdPtr->wan.data_offset);
        shell_print(shell_priv, "\treason: %d\n", pdPtr->wan.reason);
        shell_print(shell_priv, "\tsource port: %d\n", pdPtr->wan.source_port);
        shell_print(shell_priv, "\tWAN flow id: %d\n", pdPtr->wan.wan_flow_id);
    }
}

static void cpu_feed_pd_print_fields(void *shell_priv, CPU_FEED_DESCRIPTOR* pdPtr)
{
    shell_print(shell_priv, "Feed descriptor fields:\n");
    shell_print(shell_priv, "\tpacket DDR uncached address low: 0x%pK\n",(void*)PHYS_TO_UNCACHED(pdPtr->abs.host_buffer_data_ptr_low));
    shell_print(shell_priv, "\tpacket DDR uncached address hi: 0x%pK\n",(void*)PHYS_TO_UNCACHED(pdPtr->abs.host_buffer_data_ptr_hi));
    shell_print(shell_priv, "\tpacket type: %d\n", pdPtr->abs.abs);
    shell_print(shell_priv, "\treserved: 0x%x\n", pdPtr->abs.reserved);
}
#else

static void cpu_ring_pd_print_fields(void *shell_priv, CPU_RX_DESCRIPTOR* pdPtr)
{
    shell_print(shell_priv, "descriptor fields:\n");
    shell_print(shell_priv, "\townership: %s\n", pdPtr->ownership ? "Host":"Runner");
    shell_print(shell_priv, "\tpacket DDR uncached address: 0x%pK\n", (void*)PHYS_TO_UNCACHED(pdPtr->host_buffer_data_pointer));
    shell_print(shell_priv, "\tpacket len: %d\n", pdPtr->packet_length);

    if(pdPtr->ownership != OWNERSHIP_HOST)
        return;

    shell_print(shell_priv, "\tdescriptor type: %d\n", pdPtr->descriptor_type);
    shell_print(shell_priv, "\tsource port: %d\n", pdPtr->source_port );
    shell_print(shell_priv, "\tflow_id: %d\n", pdPtr->flow_id);
    shell_print(shell_priv, "\twl chain id: %d\n", pdPtr->wl_metadata & 0xFF);
    shell_print(shell_priv, "\twl priority: %d\n", (pdPtr->wl_metadata & 0xFF00) >> 8);
}
#endif

int cpu_ring_shell_print_pd(void *shell_priv, uint32_t ring_id, uint32_t pdIndex)
{
#ifdef XRDP
    if (host_ring[ring_id].type != rdpa_ring_feed)
    {
#endif
        CPU_RX_DESCRIPTOR host_ring_desc;

#ifndef XRDP
        memcpy(&host_ring_desc, (CPU_RX_DESCRIPTOR *)host_ring[ring_id].base + pdIndex, sizeof(CPU_RX_DESCRIPTOR));
#else
        memcpy(&host_ring_desc, &((CPU_RX_DESCRIPTOR *)host_ring[ring_id].base)[pdIndex],
            sizeof(CPU_RX_DESCRIPTOR));
#endif

        shell_print(shell_priv, "descriptor unswapped: %08x %08x %08x %08x\n",
            host_ring_desc.word0, host_ring_desc.word1, host_ring_desc.word2,
            host_ring_desc.word3 );

        host_ring_desc.word0 = swap4bytes(host_ring_desc.word0);
        host_ring_desc.word1 = swap4bytes(host_ring_desc.word1);
        host_ring_desc.word2 = swap4bytes(host_ring_desc.word2);
        host_ring_desc.word3 = swap4bytes(host_ring_desc.word3);

        shell_print(shell_priv, "descriptor swapped  : %08x %08x %08x %08x\n", 
            host_ring_desc.word0, host_ring_desc.word1, host_ring_desc.word2,
            host_ring_desc.word3 );

        cpu_ring_pd_print_fields(shell_priv, &host_ring_desc);

        return 0;
#ifdef XRDP 
    }
    else
    {
        CPU_FEED_DESCRIPTOR host_feed_desc;

        memcpy(&host_feed_desc, &((CPU_FEED_DESCRIPTOR *)host_ring[ring_id].base)[pdIndex],
            sizeof(CPU_FEED_DESCRIPTOR));

        shell_print(shell_priv, "feed descriptor unswapped: %08x %08x\n",
            host_feed_desc.word0, host_feed_desc.word1 );

        host_feed_desc.word0 = swap4bytes(host_feed_desc.word0);
        host_feed_desc.word1 = swap4bytes(host_feed_desc.word1);

        shell_print(shell_priv, "descriptor swapped  : %08x %08x\n", 
            host_feed_desc.word0, host_feed_desc.word1 );

        cpu_feed_pd_print_fields(shell_priv, &host_feed_desc);

        return 0;
    }
#endif
}

int cpu_ring_shell_admin_ring(void *shell_priv, uint32_t ring_id, uint32_t admin_status)
{
    host_ring[ring_id].admin_status	= admin_status;

    shell_print(shell_priv, "ring_id %d admin status is set to :%s\n", ring_id, host_ring[ring_id].admin_status ? "Up" : "Down");

    return 0;
}

/*bdmf shell is compiling only for RDPA and not CFE*/
#ifdef __KERNEL__
#define MAKE_BDMF_SHELL_CMD_NOPARM(dir, cmd, help, cb) \
    bdmfmon_cmd_add(dir, cmd, cb, help, BDMF_ACCESS_ADMIN, NULL, NULL)

#define MAKE_BDMF_SHELL_CMD(dir, cmd, help, cb, parms...)   \
{                                                           \
    static bdmfmon_cmd_parm_t cmd_parms[]={                 \
        parms,                                              \
        BDMFMON_PARM_LIST_TERMINATOR                        \
    };                                                      \
    bdmfmon_cmd_add(dir, cmd, cb, help, BDMF_ACCESS_ADMIN, NULL, cmd_parms); \
}

static int bdmf_cpu_ring_shell_list_rings( bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms )
{
    int start_from = -1;

    if (n_parms == 1)
        start_from = (uint32_t)parm[0].value.unumber;

    return cpu_ring_shell_list_rings(session, start_from);
}

static int bdmf_cpu_ring_shell_print_pd(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    return cpu_ring_shell_print_pd(session, (uint32_t)parm[0].value.unumber, (uint32_t)parm[1].value.unumber);
}

static int bdmf_cpu_ring_shell_admin_ring(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    return cpu_ring_shell_admin_ring(session, (uint32_t)parm[0].value.unumber, (uint32_t)parm[1].value.unumber);
}

#define MAKE_BDMF_SHELL_CMD_NOPARM(dir, cmd, help, cb) \
    bdmfmon_cmd_add(dir, cmd, cb, help, BDMF_ACCESS_ADMIN, NULL, NULL)

#define MAKE_BDMF_SHELL_CMD(dir, cmd, help, cb, parms...)   \
{                                                           \
    static bdmfmon_cmd_parm_t cmd_parms[]={                 \
        parms,                                              \
        BDMFMON_PARM_LIST_TERMINATOR                        \
    };                                                      \
    bdmfmon_cmd_add(dir, cmd, cb, help, BDMF_ACCESS_ADMIN, NULL, cmd_parms); \
}
int ring_make_shell_commands ( void )
{
    bdmfmon_handle_t driver_dir, cpu_dir;

    if ( !( driver_dir = bdmfmon_dir_find ( NULL, "driver" ) ) )
    {
        driver_dir = bdmfmon_dir_add ( NULL, "driver", "Device Drivers", BDMF_ACCESS_ADMIN, NULL );

        if ( !driver_dir )
            return ( 1 );
    }

    cpu_dir = bdmfmon_dir_add ( driver_dir, "cpur", "CPU Ring Interface Driver", BDMF_ACCESS_ADMIN, NULL );

    if ( !cpu_dir )
        return ( 1 );


    MAKE_BDMF_SHELL_CMD( cpu_dir, "sar",   "Show available rings", bdmf_cpu_ring_shell_list_rings,
        BDMFMON_MAKE_PARM_RANGE( "ring_id", "ring id", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL, 0, D_NUM_OF_RING_DESCRIPTORS) );

    MAKE_BDMF_SHELL_CMD( cpu_dir, "vrpd",     "View Ring packet descriptor", bdmf_cpu_ring_shell_print_pd,
        BDMFMON_MAKE_PARM_RANGE( "ring_id", "ring id", BDMFMON_PARM_NUMBER, 0, 0, D_NUM_OF_RING_DESCRIPTORS ),
        BDMFMON_MAKE_PARM_RANGE( "descriptor", "packet descriptor index ", BDMFMON_PARM_NUMBER, 0, 0, RDPA_CPU_QUEUE_MAX_SIZE) );

    MAKE_BDMF_SHELL_CMD( cpu_dir, "cras",     "configure ring admin status", bdmf_cpu_ring_shell_admin_ring,
        BDMFMON_MAKE_PARM_RANGE( "ring_id", "ring id", BDMFMON_PARM_NUMBER, 0, 0, D_NUM_OF_RING_DESCRIPTORS ),
        BDMFMON_MAKE_PARM_RANGE( "admin", "ring admin status ", BDMFMON_PARM_NUMBER, 0, 0, 1) );

    return 0;
}
#endif /*__KERNEL__*/

/*delete a preallocated ring*/
int	rdp_cpu_ring_delete_ring(uint32_t ring_id)
{
    RING_DESCTIPTOR*			pDescriptor;

    pDescriptor = &host_ring[ring_id];
    if(!pDescriptor->num_of_entries)
    {
        printk("ERROR:deleting ring_id %d which does not exists!",ring_id);
        return -1;
    }

    rdp_cpu_ring_buffers_free(pDescriptor);

    /* free any buffers in buff_cache */
    while(pDescriptor->buff_cache_cnt) 
    {
        pDescriptor->databuf_free(pDescriptor->buff_cache[--pDescriptor->buff_cache_cnt], 0, pDescriptor);
    }

    /*free buff_cache */
    if(pDescriptor->buff_cache)
        CACHED_FREE(pDescriptor->buff_cache);

    /*delete the ring of descriptors*/
    if(pDescriptor->base)
        rdp_mm_aligned_free((void*)NONCACHE_TO_CACHE(pDescriptor->base),
            pDescriptor->num_of_entries * pDescriptor->size_of_entry);

    pDescriptor->num_of_entries = 0;

    return 0;
}
EXPORT_SYMBOL(rdp_cpu_ring_delete_ring);

/* Using void * instead of (rdpa_cpu_rxq_ic_cfg_t *) to avoid CFE compile errors*/
int	rdp_cpu_ring_create_ring(uint32_t ring_id,
    uint8_t ring_type,
    uint32_t entries,
    bdmf_phys_addr_t* ring_head,
    uint32_t packetSize,
    RING_CB_FUNC* ringCb)
{
    RING_DESCTIPTOR* pDescriptor;
    bdmf_phys_addr_t phy_addr;

    if (ring_id >= RING_ID_NUM_OF)
    {
        printk("ERROR: ring_id %d out of range(%d)", ring_id, RING_ID_NUM_OF);
        return -1;
    }

#ifdef XRDP
    if (ring_type == rdpa_ring_feed)
        pDescriptor = &host_ring[FEED_RING_ID];
    else if (ring_type == rdpa_ring_recycle)
        pDescriptor = &host_ring[RCYCLE_RING_ID];
    else
#endif
        pDescriptor = &host_ring[ring_id];

    if(pDescriptor->num_of_entries)
    {
        printk("ERROR: ring_id %d already exists! must be deleted first",ring_id);
        return -1;
    }

    if(!entries)
    {
        printk("ERROR: can't create ring with 0 packets\n");
        return -1;
    }

    printk("Creating CPU ring for queue number %d with %d packets descriptor=0x%p\n ",ring_id,entries,pDescriptor);

    /*set ring parameters*/

    pDescriptor->ring_id = ring_id;
    pDescriptor->admin_status = 1;
    pDescriptor->num_of_entries = entries;

#ifdef XRDP
    if (ring_type == rdpa_ring_feed)
        pDescriptor->size_of_entry		= sizeof(CPU_FEED_DESCRIPTOR);
    else
#endif
        pDescriptor->size_of_entry		= sizeof(CPU_RX_DESCRIPTOR);

    pDescriptor->buff_cache_cnt = 0;
    pDescriptor->packet_size = packetSize;
    pDescriptor->type = ring_type;

    pDescriptor->databuf_alloc  = rdp_databuf_alloc;
    pDescriptor->databuf_free   = rdp_databuf_free;
    pDescriptor->data_dump = rdp_packet_dump;
    
    if (ringCb) /* overwrite if needed */
    {
        pDescriptor->data_dump = ringCb->data_dump;
        pDescriptor->buff_mem_context = ringCb->buff_mem_context;
#ifndef XRDP
        pDescriptor->databuf_alloc  = ringCb->databuf_alloc;
        pDescriptor->databuf_free   = ringCb->databuf_free;
#endif
    }

    /*TODO:update the comment  allocate buff_cache which helps to reduce the overhead of when 
     * allocating data buffers to ring descriptor */
    pDescriptor->buff_cache = (uint8_t **)(CACHED_MALLOC_ATOMIC(sizeof(uint8_t *) * MAX_BUFS_IN_CACHE));
    if( pDescriptor->buff_cache == NULL )
    {
        printk("failed to allocate memory for cache of data buffers \n");
        return -1;
    }

    /*allocate ring descriptors - must be non-cacheable memory*/
    pDescriptor->base = (void *)rdp_mm_aligned_alloc(pDescriptor->size_of_entry * entries, &phy_addr);
    if( pDescriptor->base == NULL)
    {
        printk("failed to allocate memory for ring descriptor\n");
        rdp_cpu_ring_delete_ring(ring_id);
        return -1;
    }

    if (rdp_cpu_ring_buffers_init(pDescriptor, ring_id))
        return -1;

#ifndef XRDP
    /*set the ring header to the first entry*/
    pDescriptor->head = (CPU_RX_DESCRIPTOR *)pDescriptor->base;

    /*using pointer arithmetics calculate the end of the ring*/
    pDescriptor->end = (CPU_RX_DESCRIPTOR *)pDescriptor->base + entries;
#endif

    *ring_head = phy_addr;

#ifndef XRDP
    printk("Done initializing Ring %d Base=0x%pK End=0x%pK "
        "calculated entries= %ld RDD Base=%lxK descriptor=0x%p\n", ring_id,
        pDescriptor->base, pDescriptor->end, (long)(pDescriptor->end - (CPU_RX_DESCRIPTOR *)pDescriptor->base), 
        (unsigned long)phy_addr, pDescriptor);
#else
    printk("Done initializing Ring %d Base=0x%pK num of entries= %d RDD Base=%lxK descriptor=0x%p\n",
        ring_id, pDescriptor->base, pDescriptor->num_of_entries, (unsigned long)phy_addr, pDescriptor);
#endif

#ifdef __KERNEL__
    {
        if(!init_shell)
        {
            if(ring_make_shell_commands())
            {	printk("Failed to create ring bdmf shell commands\n");
                return 1;
            }

            init_shell = 1;
        }
    }
#endif
    return 0;
}
EXPORT_SYMBOL(rdp_cpu_ring_create_ring);

void rdp_cpu_ring_free_mem(uint32_t ringId, void *pBuf)
{
    RING_DESCTIPTOR *pDescriptor     = &host_ring[ringId];

    if ((pDescriptor == NULL) || (pDescriptor->databuf_free == NULL))
    {
        printk("rdp_cpu_ring_free_mem: pDescriptor or free_cb is NULL, Memory is not freed.\n");
        return;
    }

    pDescriptor->databuf_free(pBuf, 0, pDescriptor);
}
EXPORT_SYMBOL(rdp_cpu_ring_free_mem);

#ifdef __KERNEL__

inline int rdp_cpu_ring_get_packet(uint32_t ringId, rdpa_cpu_rx_info_t *info)
{
    int rc;
    CPU_RX_PARAMS params;
    rdpa_traffic_dir dir;
    RING_DESCTIPTOR *pDescriptor     = &host_ring[ringId];

    //Check ringId range

    if (pDescriptor == NULL)
    {
        printk("rdp_cpu_ring_get_packet: pDescriptor is NULL\n");
        return BDMF_ERR_INTERNAL;
    }

    memset((void *)&params, 0, sizeof(CPU_RX_PARAMS));

    rc = rdp_cpu_ring_read_packet_refill(ringId, &params);
    if (rc)
    {
#ifdef LEGACY_RDP
        if (rc == BL_LILAC_RDD_ERROR_CPU_RX_QUEUE_EMPTY)
            return BDMF_ERR_NO_MORE;
#else
        if (rc == BDMF_ERR_NO_MORE)
            return rc;
#endif
        return BDMF_ERR_INTERNAL;
    }

#ifndef XRDP
    info->src_port = rdpa_cpu_rx_srcport_to_rdpa_if(params.src_bridge_port,
        params.flow_id);
    if (info->src_port == rdpa_if_none)
    {
        pDescriptor->stats_dropped++;

        pDescriptor->databuf_free(params.data_ptr, 0, pDescriptor);

        return BDMF_ERR_PERM;
    }
#else
    info->vport = params.src_bridge_port;
    info->src_port = rdpa_port_vport_to_rdpa_if(params.src_bridge_port);
    info->dest_ssid = params.dst_ssid;
#endif

    info->reason = (rdpa_cpu_reason)params.reason;
    info->reason_data = params.flow_id;
    info->ptp_index = params.ptp_index;
    info->data = (void*)params.data_ptr;
#ifdef XRDP
    info->data_offset = params.data_offset;
#endif
    info->size = params.packet_size;
    dir = rdpa_if_is_wan(info->src_port) ? rdpa_dir_ds : rdpa_dir_us;
    stats_reason[dir][info->reason]++;

    pDescriptor->stats_received++;
    if (unlikely(pDescriptor->dump_enable))
        pDescriptor->data_dump(pDescriptor->ring_id, info);

    return rc;
}
EXPORT_SYMBOL(rdp_cpu_ring_get_packet);

void rdp_cpu_dump_data_cb(bdmf_index queue, bdmf_boolean enabled)
{
    host_ring[queue].dump_enable = enabled;
}
EXPORT_SYMBOL(rdp_cpu_dump_data_cb);

void rdp_cpu_rxq_stat_cb(int qid, extern_rxq_stat_t *stat, bdmf_boolean clear)
{
    RING_DESCTIPTOR *pDescriptor     = &host_ring[qid];

    if (!stat)
        return;

    stat->received = pDescriptor->stats_received;
    stat->dropped = pDescriptor->stats_dropped;
    stat->queued  = rdp_cpu_ring_get_queued(qid);

    if (clear)
        pDescriptor->stats_received = pDescriptor->stats_dropped = 0;
}
EXPORT_SYMBOL(rdp_cpu_rxq_stat_cb);

void rdp_cpu_reason_stat_cb(uint32_t *stat, rdpa_cpu_reason_index_t *rindex)
{
    if ((!stat) || (!rindex))
        return;

    *stat = stats_reason[rindex->dir][rindex->reason];
    stats_reason[rindex->dir][rindex->reason] = 0;
}
EXPORT_SYMBOL(rdp_cpu_reason_stat_cb);

#endif //(__KERNEL__)

#if defined(_CFE_) || !(defined(CONFIG_BCM963138) || defined(_BCM963138_) || defined(CONFIG_BCM963148) || defined(_BCM963148_))
/*this API copies the next available packet from ring to given pointer*/
int rdp_cpu_ring_read_packet_copy( uint32_t ring_id, CPU_RX_PARAMS* rxParams)
{
    RING_DESCTIPTOR* 					pDescriptor		= &host_ring[ ring_id ];
#ifndef XRDP
    volatile CPU_RX_DESCRIPTOR*			pTravel = (volatile CPU_RX_DESCRIPTOR*)pDescriptor->head;
#endif
    void* 	 						    client_pdata;
    uint32_t 							ret = 0;

    /* Data offset field is field ONLY in CFE driver on BCM6858 
     * To ensure correct work of another platforms the data offset field should be zeroed */
    rxParams->data_offset = 0;

    client_pdata 		= (void*)rxParams->data_ptr;

#ifndef XRDP
    ret = ReadPacketFromRing(pDescriptor,pTravel, rxParams);
#else
    ret = ReadPacketFromRing(pDescriptor, rxParams);
#endif
    if ( ret )
        goto exit;

    /*copy the data to user buffer*/
    /*TODO: investigate why INV_RANGE is needed before memcpy,*/  
    INV_RANGE((rxParams->data_ptr + rxParams->data_offset), rxParams->packet_size);
    memcpy(client_pdata,(void*)(rxParams->data_ptr + rxParams->data_offset), rxParams->packet_size);

    /*Assign the data buffer back to ring*/
    INV_RANGE((rxParams->data_ptr + rxParams->data_offset), rxParams->packet_size);
#ifndef XRDP
    AssignPacketBuffertoRing(pDescriptor, pTravel, rxParams->data_ptr);
#else
    AssignPacketBuffertoRing(pDescriptor, rxParams->data_ptr);
#endif

exit:
    rxParams->data_ptr = client_pdata;
    return ret;
}
#endif

/*this function if for debug purposes*/
int	rdp_cpu_ring_get_queue_size(uint32_t ring_idx)
{
    return host_ring[ ring_idx ].num_of_entries;
}


/*this function if for debug purposes and should not be called during runtime*/
/*TODO:Add mutex to protect when reading while packets read from another context*/
int	rdp_cpu_ring_get_queued(uint32_t ring_idx)
{
    RING_DESCTIPTOR*			pDescriptor = &host_ring[ ring_idx ];
    uint32_t                    packets     = 0;
#ifndef XRDP
    volatile CPU_RX_DESCRIPTOR*	pTravel		= pDescriptor->base;
    volatile CPU_RX_DESCRIPTOR*	pEnd		= pDescriptor->end;
#else
    uint32_t read_idx = 0, write_idx = 0;
    rdp_cpu_get_read_idx(pDescriptor->ring_id, pDescriptor->type, &read_idx);
    rdd_cpu_get_write_idx(pDescriptor->ring_id, pDescriptor->type, &write_idx);
#endif

    if(pDescriptor->num_of_entries == 0)
        return 0;
#ifndef XRDP
    while (pTravel != pEnd)
    {
        if (rdp_cpu_ring_is_ownership_host(pTravel))
            packets++;
        pTravel++;
    }
#else
    if (read_idx <= write_idx)
        packets = write_idx - read_idx;
    else 
        packets = (pDescriptor->num_of_entries - read_idx) + write_idx;
#endif

    return packets;
}

int	rdp_cpu_ring_flush(uint32_t ring_id)
{
    RING_DESCTIPTOR*			pDescriptor = &host_ring[ ring_id ];
#ifndef XRDP
    volatile CPU_RX_DESCRIPTOR*	pTravel		= pDescriptor->base;
    volatile CPU_RX_DESCRIPTOR*	pEnd		= pDescriptor->end;

    while (pTravel != pEnd)
    {
        rdp_cpu_ring_set_ownership_runner(pTravel);
        pTravel++;
    }

    pDescriptor->head = (CPU_RX_DESCRIPTOR *)pDescriptor->base;
#else
    if (host_ring[ring_id].type != rdpa_ring_feed)
        rdp_cpu_ring_buffers_free(pDescriptor);
#endif
    printk("cpu Ring %d has been flushed\n",ring_id);

    return 0;
}

int	rdp_cpu_ring_not_empty(uint32_t ring_id)
{
    RING_DESCTIPTOR*			pDescriptor = &host_ring[ ring_id ];
#ifndef XRDP
    CPU_RX_DESCRIPTOR*	pTravel		= (pDescriptor->head );

    return rdp_cpu_ring_is_ownership_host(pTravel);
#else
    uint32_t read_idx =0, write_idx = 0;

    read_idx = pDescriptor->shadow_read_idx;
    write_idx = pDescriptor->shadow_write_idx;

    return read_idx != write_idx ? 1 : 0;
#endif
}

int rdp_cpu_ring_is_full(uint32_t ring_id)
{
#ifdef XRDP
    RING_DESCTIPTOR*          pDescriptor = &host_ring[ ring_id ];

    return (pDescriptor->num_of_entries - rdp_cpu_ring_get_queued(ring_id) < 10);


#else
    printk("%s NOT IMPLEMENTED \n",__FUNCTION__);
    return 0;
#endif
}
#ifndef _CFE_
/*this API get the pointer of the next available packet and reallocate buffer in ring
 * in the descriptor is optimized to 16 bytes cache line, 6838 has 16 bytes cache line
 * while 68500 has 32 bytes cache line, so we don't prefetch the descriptor to cache
 * Also on ARM platform we are not sure of how to skip L2 cache, and use only L1 cache
 * so for now  always use uncached accesses to Packet Descriptor(pTravel)
 */

inline int rdp_cpu_ring_read_packet_refill(uint32_t ring_id, CPU_RX_PARAMS *rxParams)
{
    uint32_t                     ret;
    RING_DESCTIPTOR *pDescriptor     = &host_ring[ring_id];

#ifndef XRDP
    volatile CPU_RX_DESCRIPTOR* pTravel = (volatile CPU_RX_DESCRIPTOR*)pDescriptor->head;
#endif

    void *pNewBuf = NULL;

#ifdef __KERNEL__
    if (unlikely(pDescriptor->admin_status == 0))
    {
#ifndef LEGACY_RDP
        return BDMF_ERR_NO_MORE;
#else
        return BL_LILAC_RDD_ERROR_CPU_RX_QUEUE_EMPTY;
#endif
    }
#endif

#ifndef XRDP
    ret = ReadPacketFromRing(pDescriptor, pTravel, rxParams);
#else
    ret = ReadPacketFromRing(pDescriptor, rxParams);
#endif

    if (ret)
    {
        return  ret;
    }

#ifdef BCM6858
    bdmf_dcache_inv((unsigned long)(rxParams->data_ptr + rxParams->data_offset), rxParams->packet_size);
#endif

#ifndef XRDP
    /* A valid packet is recieved try to allocate a new data buffer and
     * refill the ring before giving the packet to upper layers
     */

    pNewBuf  = pDescriptor->databuf_alloc(pDescriptor);

    /*validate allocation*/
    if (unlikely(!pNewBuf))
    {
        //printk("ERROR:system buffer allocation failed!\n");
        /*assign old data buffer back to ring*/
        pNewBuf   = rxParams->data_ptr;
        rxParams->data_ptr = NULL;
        ret = 1;
    }

    AssignPacketBuffertoRing(pDescriptor, pTravel, pNewBuf);
#else
    /* In XRDP implementation this API will raise the thread on threashold that will fill feed ring */
    AssignPacketBuffertoRing(pDescriptor, pNewBuf);
#endif

    return ret;
}

#ifdef XRDP
/* interrupt routine Recycle ring*/
static int _rdp_cpu_ring_recycle_free_host_buf(void)
{
    RING_DESCTIPTOR *ring_descr = &host_ring[RCYCLE_RING_ID];
    volatile CPU_RX_DESCRIPTOR * cpu_recycle_descr;
    CPU_RX_DESCRIPTOR rx_desc;
    uintptr_t phys_ptr;
    void* data_ptr;
    uint32_t read_idx = ring_descr->shadow_read_idx;
    uint32_t write_idx =ring_descr->shadow_write_idx;


    if (unlikely(read_idx == write_idx))
    {
            return BDMF_ERR_NO_MORE;
    }

    cpu_recycle_descr = &((CPU_RX_DESCRIPTOR *)ring_descr->base)[read_idx];

    /* Read the ownership bit first */
    rx_desc.word0 = swap4bytes(cpu_recycle_descr->word0);
    rx_desc.word1 = swap4bytes(cpu_recycle_descr->word1);

    phys_ptr = ((uintptr_t)rx_desc.abs.host_buffer_data_ptr_hi) << 32;
    phys_ptr |= rx_desc.abs.host_buffer_data_ptr_low;
    data_ptr = (void *)RDD_PHYS_TO_VIRT(phys_ptr);
    bdmf_sysb_free(data_ptr);

    ring_descr->shadow_read_idx = (++read_idx)%ring_descr->num_of_entries;

    return BDMF_ERR_OK;
}

int rdp_cpu_ring_recycle_free_host_buf(int budget)
{
    RING_DESCTIPTOR *ring_descr = &host_ring[RCYCLE_RING_ID];
    int rc = 0;
    int i;

    /* Update with real value*/
    rdd_cpu_get_write_idx(ring_descr->ring_id, rdpa_ring_recycle, &ring_descr->shadow_write_idx);

    for (i = 0; i < budget; i++)
    {
        rc = _rdp_cpu_ring_recycle_free_host_buf();

        if (rc)
            break;
    }

    rdd_cpu_inc_read_idx(0, rdpa_ring_recycle, i);
    return i;
}
EXPORT_SYMBOL(rdp_cpu_ring_recycle_free_host_buf);

int rdp_cpu_fill_feed_ring(int budget)
{
    RING_DESCTIPTOR *feed_ring_descr = &host_ring[FEED_RING_ID];
    int rc = 0;
    int i;

    rdp_cpu_get_read_idx(FEED_RING_ID, rdpa_ring_feed, &feed_ring_descr->shadow_read_idx);

    for (i = 0; i < budget; i++)
    {
        rc = alloc_and_assign_packet_to_feed_ring();
        if (rc)
            break;
    }

    rdd_cpu_inc_feed_ring_write_idx(i);
    return i;
}
EXPORT_SYMBOL(rdp_cpu_fill_feed_ring);

#endif
#endif /*ifndef _CFE_*/

/* Callback Functions */

void rdp_packet_dump(uint32_t ringId, rdpa_cpu_rx_info_t *info)
{
    char name[10];

    sprintf(name, "Queue-%d", ringId);
#ifdef __KERNEL__
    rdpa_cpu_rx_dump_packet(name, rdpa_cpu_host, ringId, info, 0);
#endif
}
EXPORT_SYMBOL(rdp_packet_dump);

#if defined(__KERNEL__)

/* BPM */

void* rdp_databuf_alloc(RING_DESCTIPTOR *pDescriptor)
{
    if (likely(pDescriptor->buff_cache_cnt))
    {
        return (void *)(pDescriptor->buff_cache[--pDescriptor->buff_cache_cnt]);
    }
    else
    {
        uint32_t alloc_cnt = 0;

        /* refill the local cache from global pool */
        alloc_cnt = bdmf_sysb_databuf_alloc((void **)pDescriptor->buff_cache, MAX_BUFS_IN_CACHE, 0);

        if (alloc_cnt)
        {
            pDescriptor->buff_cache_cnt = alloc_cnt;
            return (void *)(pDescriptor->buff_cache[--pDescriptor->buff_cache_cnt]);
        }
    }
    return NULL;
}
EXPORT_SYMBOL(rdp_databuf_alloc);

void rdp_databuf_free(void *pBuf, uint32_t context, RING_DESCTIPTOR *pDescriptor)
{
    bdmf_sysb_databuf_free(pBuf, context);
}
EXPORT_SYMBOL(rdp_databuf_free);

/* Kmem_Cache */

void* rdp_databuf_alloc_cache(RING_DESCTIPTOR *pDescriptor)
{
    if (likely(pDescriptor->buff_cache_cnt))
    {
        return (void *)(pDescriptor->buff_cache[--pDescriptor->buff_cache_cnt]);
    }
    else
    {
        uint32_t alloc_cnt = 0;
        int i;

        /* refill the local cache from global pool */
        for (i=0; i<MAX_BUFS_IN_CACHE; i++, alloc_cnt++)
        {
            uint8_t *datap;

            /* allocate from kernel directly */
            datap = kmem_cache_alloc((struct kmem_cache*)(pDescriptor->buff_mem_context), GFP_ATOMIC);

            /* do a cache invalidate of the buffer */
            bdmf_dcache_inv((unsigned long)datap, pDescriptor->packet_size );

            pDescriptor->buff_cache[i] = datap;
        }

        if (alloc_cnt)
        {
            pDescriptor->buff_cache_cnt = alloc_cnt;
            return (void *)(pDescriptor->buff_cache[--pDescriptor->buff_cache_cnt]);
        }
    }
    return NULL;
}
EXPORT_SYMBOL(rdp_databuf_alloc_cache);


void rdp_databuf_free_cache(void *pBuf, uint32_t context, RING_DESCTIPTOR *pDescriptor)
{
    kmem_cache_free((struct kmem_cache*)(pDescriptor->buff_mem_context), pBuf);
}
EXPORT_SYMBOL(rdp_databuf_free_cache);

#elif defined(_CFE_)

void* rdp_databuf_alloc(RING_DESCTIPTOR *pDescriptor)
{
    void *pBuf = KMALLOC(BCM_PKTBUF_SIZE, 16);

    if (pBuf)
    {
        INV_RANGE(pBuf, BCM_PKTBUF_SIZE);
        return pBuf;
    }
    return NULL;
}

void rdp_databuf_free(void *pBuf, uint32_t context, RING_DESCTIPTOR *pDescriptor)
{
    KFREE(pBuf);
}

#endif

#endif