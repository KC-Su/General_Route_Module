/*!
	@file
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "groute.h"
#include "glist.h"
#include "jhash.h"

#define ADDR_LEN(t)   (t==AF_INET) ? 4 : 16

#ifdef GROUTE_UNIT_TEST
#include "groute_test.h"
#else
#define INJECT_FUNC(origin, stub) origin
#endif

/*!
    \brief  This is Groute internal data structure to represent an interface
	
	Any interface configuration will be stored in this data structutre. It was linked to g_intf_free_list
	after initialization, and linked to g_intf_used_list when used. An interface was configured with IPv4/IPv6
	address, it store any known gateway belongs to the network.
*/
typedef struct {
	struct glist_head list_node;	//!< link to free list or used list
	struct glist_head gw_node;		//!< link all gateway which is belong the network 
	uint8_t 	use:1;				//!< this node is add to used list or not
	uint8_t 	link:1;				//!< admin link up/down
	uint8_t 	ipv4_flag:1;		//!< ipv4 address set
	uint8_t 	ipv6_flag:1;		//!< ipv6 address set
	uint8_t 	res:4;				//!< reserved flags
	char 		intf_name[GROUTE_INTF_NAME_SIZE]; //!< Store interface name that configuration giving
	uint8_t 	ipv4_addr[4];		//!< ipv4 address
	uint8_t 	ipv4_prefix;		//!< ipv4 address mask length
	uint8_t 	ipv6_addr[16];		//!< ipv6 address
	uint8_t 	ipv6_prefix;		//!< ipv6 address mask length
} groute_intf_t;

typedef struct groute_route_s groute_route_t;

/*!
    \brief  This is Groute internal data structure to represent a gateway
	
	Gateways is be seems a part of route with limited count, and it will have link to interface 
	when it has output network. This connection will be presented by find_network flag. 
*/
typedef struct {
	struct glist_head gw_node;  	//!< link to the output network if found or g_gw_idle_list
	uint8_t 	use:1;				//!< this gateway is valid in route
	uint8_t 	find_network:1; 	//!< this gateway find output interface
	uint8_t 	res:6;				//!< reserved flags
	uint8_t 	family;				//!< this gateway store which IP protocol address
    uint8_t     addr[16];			//!< gateway address which IPv4/IPv6 shared
    uint8_t     weight;				//!< valid for ECMP or just 0 to static route
	char        out_intf_name[GROUTE_INTF_NAME_SIZE]; //!< valid when find_network flag set
	groute_route_t *belongs_route;	//!< link to the route this gateway belongs to
} groute_gw_t;


/*!
    \brief  This is Groute internal data structure to represent a route
	
	A route configuration will be stored in this data structure. It was linked to g_route_free_list
	after initialization, and linked to hash list stored in SHM when used.
*/
typedef struct groute_route_s {
	struct glist_head list_node;	//!< link to free list or hash list
	uint8_t 	use:1;				//!< this route is valid
	uint8_t 	gateway_has_nw:1;	//!< one of the route gateway found output network
	uint8_t 	ecmp_flag:1;		//!< this route is ecmp route
	uint8_t 	res:5;				//!< reserved flags
	uint8_t 	af_family;			//!< this route is IPv4/IPv6 route
	uint32_t 	metric;				//!< route mertic
	uint8_t 	dst_addr[16];		//!< destination address of the route
	uint8_t 	prefix;				//!< destination address prefix of the route
	uint8_t 	nb_gw;				//!< how many gateway is configured
	groute_gw_t gateway[GROUTE_GW_NB]; //!< continuous valid gateway information stored with max GROUTE_GW_NB gateway
} groute_route_t;

// ROUTE shm composed by 4 parts
// 1. 1 * uint8_t lock
// 2. 1 * uint64_t (memory base address to the process which calling groute_reload)
// 3. GROUTE_HASH_SIZE * sizeof(struct glist_head)
// 4. GROUTE_ROUTE_NB * sizeof(groute_route_t)
typedef struct {
	uint8_t 			lock;
	uint64_t			base_addr;
	struct glist_head 	hash_list[GROUTE_HASH_SIZE];
	groute_route_t 		route_pool[GROUTE_ROUTE_NB];
} groute_route_shm_t;

static int 	g_cb_valid;
static groute_conf_cb_t g_kernel_cb, g_user_cb;
static struct glist_head g_gw_idle_list;
static struct glist_head g_intf_free_list, g_intf_used_list;
static struct glist_head g_route_free_list;
static groute_route_shm_t *g_route_shm = (groute_route_shm_t *)(-1), *g_reload_dump_route_shm = (groute_route_shm_t *)(-1);
static groute_intf_t *g_intf_shm = (groute_intf_t *)(-1), *g_reload_dump_intf_shm = (groute_intf_t *)(-1);
// Private funciton

/*!
<pre>
	This private function is try to lock the resource implement by C built-in atomic operation
</pre>
	@param[in] lock The lock try to be locked

	@return NULL
	@author Tim Su
*/
static void __lock(uint8_t *lock)
{
	while(1) {
		if (__sync_bool_compare_and_swap(lock, (uint8_t)0, (uint8_t)1)) {
			break;
		}
	}
	return;
}

/*!
<pre>
	This private function is try to unlock the resource implement by C built-in atomic operation
</pre>
	@param[in] lock The lock try to be unlocked

	@return NULL
	@author Tim Su
*/
static void __unlock(uint8_t *lock)
{
	while(1) {
		if (__sync_bool_compare_and_swap(lock, (uint8_t)1, (uint8_t)0)) {
			break;
		}
	}
	return;
}

/*!
<pre>
	This private function is responsible to initial interface resource
</pre>
	@return NULL
	@author Tim Su
*/
static void __init_intf_pool()
{
	int i;

	INIT_GLIST_HEAD(&g_intf_free_list);
	INIT_GLIST_HEAD(&g_intf_used_list);
	for (i = 0; i < GROUTE_INTF_NB; i++) {
		INIT_GLIST_HEAD(&g_intf_shm[i].list_node);
		INIT_GLIST_HEAD(&g_intf_shm[i].gw_node);
		glist_add(&g_intf_shm[i].list_node ,&g_intf_free_list);
	}
	return;
}

/*!
<pre>
	This private function is responsible to initial route resource
</pre>
	@return NULL
	@author Tim Su
*/
static void __init_route_pool()
{
	int i, j;
	// assign memory base
	g_route_shm->base_addr = (uint64_t)g_route_shm;

	// initialize hash array
	for (i = 0; i < GROUTE_HASH_SIZE; i++) {
		INIT_GLIST_HEAD(&g_route_shm->hash_list[i]);
	}

	// add all route node to free list
	INIT_GLIST_HEAD(&g_route_free_list);
	INIT_GLIST_HEAD(&g_gw_idle_list);
	for (i = 0; i < GROUTE_ROUTE_NB; i++) {
		INIT_GLIST_HEAD(&g_route_shm->route_pool[i].list_node);
		glist_add(&g_route_shm->route_pool[i].list_node, &g_route_free_list);
		for (j = 0; j < GROUTE_GW_NB; j++) {
			INIT_GLIST_HEAD(&g_route_shm->route_pool[i].gateway[j].gw_node);
		}
	}
	return;
}

/*!
<pre>
	This private function is responsible to check any configured interface with same interface name
</pre>
	@param[in] intf_name interface name
	@return NULL if not find, or groute_intf_t poiner point to the interface with same interface name
	@author Tim Su
*/
static groute_intf_t *__intf_lookup(char *intf_name)
{
	groute_intf_t *intf_ptr;
	glist_for_each_entry(intf_ptr, &g_intf_used_list, list_node) {
		if (!memcmp(intf_ptr->intf_name, intf_name, GROUTE_INTF_NAME_SIZE)) {
			return intf_ptr;
		}
	}

	return NULL;
}

/*!
<pre>
	This private function is responsible to check any configured route with same destination address,
	prefix, and metric.
</pre>
	@param[in] addr pointer point to destination address
	@param[in] prefix route prefix
	@param[in] metric route metric
	@return NULL if not find, or groute_route_t poiner point to the route with same  destination address,
			prefix, and metric.
	@author Tim Su
*/
static groute_route_t *__route_lookup(uint8_t *addr, uint8_t prefix, uint32_t metric)
{
	// return ptr if find the same route with same metric, otehrwise NULL
	uint32_t hash_value;
	groute_route_t *route_ptr;
	hash_value = (uint32_t)(jhash(addr, 16, 0) % GROUTE_HASH_SIZE);

	glist_for_each_entry(route_ptr, &g_route_shm->hash_list[hash_value], list_node) {
		if (memcmp(route_ptr->dst_addr, addr, 16)) {
			continue;
		}

		if (route_ptr->prefix != prefix) {
			continue;
		}

		if (route_ptr->metric == metric) {
			return route_ptr;
		}
	}

	return NULL;
}

/*!
<pre>
	This private function is responsible to get a new interface from memory pool
</pre>
	@param[in] intf_name pointer point to interface name
	@return NULL if not pool is empty, or groute_intf_t poiner point to the got interface
	@author Tim Su
*/
static groute_intf_t *__intf_get(char *intf_name)
{
	groute_intf_t *intf_ptr;

	if (glist_empty(&g_intf_free_list)) {
		return NULL;
	}

	intf_ptr = glist_entry(g_intf_free_list.next, groute_intf_t, list_node);
	glist_del_init(&intf_ptr->list_node);
	intf_ptr->use 	= 1;
	intf_ptr->link 	= 1;
	memcpy(intf_ptr->intf_name, intf_name, GROUTE_INTF_NAME_SIZE);
	glist_add(&intf_ptr->list_node, &g_intf_used_list);
	return intf_ptr;
}

/*!
<pre>
	This private function is responsible to link route to hash list
</pre>
	@param[in] route groute_route_t pointer point to route
	@author Tim Su
*/
static void __route_add_hash(groute_route_t *route)
{
	uint32_t hash_value;
	hash_value = (uint32_t)(jhash(route->dst_addr, 16, 0) % GROUTE_HASH_SIZE);

	glist_add(&route->list_node, &g_route_shm->hash_list[hash_value]);

	return;
}

/*!
<pre>
	This private function is responsible to get a new route from memory pool
</pre>
	@param[in] addr pointer point to route destination address
	@param[in] prefix route destination address prefix
	@param[in] metric route metric
	@param[in] family notify route is in IPv4/IPv6 procotol
	@param[in] nb_gw the valid number of gateway
	@return NULL if not pool is empty, or groute_route_t poiner point to the got interface
	@author Tim Su
*/
static groute_route_t *__route_get(uint8_t *addr, uint8_t prefix, uint32_t metric, uint8_t family, uint8_t nb_gw)
{
	groute_route_t *route_ptr;
	if (glist_empty(&g_route_free_list)) {
		return NULL;
	}

	route_ptr = glist_entry(g_route_free_list.next, groute_route_t, list_node);
	memcpy(route_ptr->dst_addr, addr, 16);
	route_ptr->use 	  = 1;
	route_ptr->prefix = prefix;
	route_ptr->metric = metric;
	route_ptr->nb_gw  = nb_gw;
	route_ptr->af_family = family;
	glist_del_init(&route_ptr->list_node);
	return route_ptr;
}

/*!
<pre>
	This private function is responsible to put interface back to memory pool
</pre>
	@param[in] intf_ptr groute_intf_t pointer point to interface
	@author Tim Su
*/
static void __intf_put(groute_intf_t *intf_ptr)
{
	intf_ptr->use 				= 0;
	intf_ptr->link 				= 0;
	intf_ptr->ipv4_flag			= 0;
	intf_ptr->ipv6_flag 		= 0;
	intf_ptr->ipv4_prefix 		= 0;
	intf_ptr->ipv6_prefix 		= 0;
	memset(intf_ptr->intf_name, 0, GROUTE_INTF_NAME_SIZE);
	memset(intf_ptr->ipv4_addr, 0, 4);
	memset(intf_ptr->ipv6_addr, 0, 16);
	glist_del_init(&intf_ptr->list_node);
	glist_del_init(&intf_ptr->gw_node);
	glist_add(&intf_ptr->list_node, &g_intf_free_list);
	return;
}


/*!
<pre>
	This private function is responsible to put route back to memory pool
</pre>
	@param[in] route groute_route_t pointer point to route
	@author Tim Su
*/
static void __route_put(groute_route_t *route)
{
	int i;

	route->use 				= 0;
	route->gateway_has_nw 	= 0;
	route->ecmp_flag 		= 0;
	route->af_family 		= 0;
	route->metric 			= 0;
	route->prefix 			= 0;
	route->nb_gw 			= 0;
	memset(route->dst_addr, 0, 16);
	glist_del_init(&route->list_node);
	glist_add(&route->list_node, &g_route_free_list);

	for (i = 0; i < GROUTE_GW_NB; i++) {
		route->gateway[i].use 			= 0;
		route->gateway[i].find_network 	= 0;
		route->gateway[i].family 		= 0;
		route->gateway[i].weight 		= 0;
		route->gateway[i].belongs_route = 0;
		memset(route->gateway[i].addr, 0, 16);
		memset(route->gateway[i].out_intf_name, 0, GROUTE_INTF_NAME_SIZE);
		glist_del_init(&route->gateway[i].gw_node);
	}
}

/*!
<pre>
	This private function is responsible to assign interface IPv4/IPv6 address
</pre>
	@param[in] intf groute_intf_t pointer point to interface
	@param[in] family IPv4/IPv6 protocol
	@param[in] addr pointer point to address that is be expected to assign
	@param[in] prefix_len address mask length
	@author Tim Su
*/
static void __set_intf_address(groute_intf_t *intf, uint8_t family, uint8_t *addr, uint8_t prefix_len)
{
	if (family == AF_INET) {
		intf->ipv4_flag 	= 1;
		intf->ipv4_prefix 	= prefix_len;
		memcpy(intf->ipv4_addr, addr, 4);
	} else {
		intf->ipv6_flag 	= 1;
		intf->ipv6_prefix 	= prefix_len;
		memcpy(intf->ipv6_addr, addr, 16);
	}
	return;
}

/*!
<pre>
	This private function is responsible to remove interface IPv4/IPv6 address
</pre>
	@param[in] intf groute_intf_t pointer point to interface
	@param[in] family IPv4/IPv6 protocol
	@author Tim Su
*/
static void __unset_intf_address(groute_intf_t *intf, uint8_t family)
{
	if (family == AF_INET) {
		intf->ipv4_flag 	= 0;
		intf->ipv4_prefix 	= 0;
		memset(intf->ipv4_addr, 0, 4);
	} else {
		intf->ipv6_flag 	= 0;
		intf->ipv6_prefix 	= 0;
		memset(intf->ipv6_addr, 0, 16);
	}
	return;
}

/*!
<pre>
	This private function is responsible to assign all valid gateway of the route
</pre>
	@param[in] route groute_route_t pointer point to route
	@param[in] nb_gw how many valid gateway expected to assign
	@param[in] gw_desc gateway configuration information
	@author Tim Su
*/
static void __assign_gw(groute_route_t *route, uint8_t nb_gw, groute_gw_desc_t *gw_desc)
{
	int i;
	if (nb_gw > 1) {
		route->ecmp_flag = 1;
	}

	for (i = 0; i < nb_gw; i++) {
		route->gateway[i].use 	 = 1;
		route->gateway[i].family = route->af_family;
		route->gateway[i].weight = gw_desc[i].weight;
		memcpy(route->gateway[i].addr, gw_desc[i].addr, ADDR_LEN(route->af_family));
		route->gateway[i].belongs_route = route;
	}
	return;
}

/*!
<pre>
	This private function is responsible to calculate prefix length from mask
</pre>
	@param[in] family IPv4/Ipv6 protocol
	@param[in] mask expected in uint8 array format
	@return return calculating result as prefix value
	@author Tim Su
*/
static int __mask_to_prefix(uint8_t family, uint8_t *mask)
{
	int i, prefix = 0;
	int size = ADDR_LEN(family);
	uint8_t byte_read;

	for (i = 0; i < size; i++) {
		byte_read = mask[i];
		//if the left most bit of the byte is not 1, stop counting
		if (!(byte_read & 0x80)) {
			break;
		}
		//count how many consecutive 1's from left most of the byte
		while(byte_read & 0x80) {
			prefix++;
			byte_read = byte_read << 1;
		}
		//if the counting is not 8 in one byte, means encounter 0 bit
		if ((prefix & 0x7) != 0) {
			break;
		}
	}
	return prefix;
}

/*!
<pre>
	This private function is responsible to judge gateway address is belongs to a network or not
</pre>
	@param[in] gw_addr pointer point to gateway address
	@param[in] intf_addr pointer point to interface address
	@param[in] prefix interface address prefix length
	@param[in] family IPv4/Ipv6 protocol
	@return Return 1 if gateway address is belongs to interface network , otherwise 0
	@author Tim Su
*/
static int __output_nw_check(uint8_t *gw_addr, uint8_t *intf_addr, uint8_t prefix, uint8_t family)
{
	int i, match_len = 0;
	int size = ADDR_LEN(family);
	uint8_t cmp[ADDR_LEN(family)];

	for (i = 0; i < size; i++) {
		cmp[i] = ~(gw_addr[i] ^ intf_addr[i]);
	}
	match_len = __mask_to_prefix(family, cmp);
	//if gateway not match output network
	if (match_len < prefix) {
		return 0;
	}
	return 1;
}


/*!
<pre>
	This private function is responsible to call user/kernel callback function when there is
	gateway finding output network
</pre>
	@param[in] intf groute_intf_t pointer point to interface
	@param[in] route groute_route_t pointer point to route
	@param[in] gw groute_gw_t pointer point to the gateway that finding output network 
	@param[in] reason to illustrated what configuration causing this result
	@author Tim Su
*/
static void __route_gw_alive(groute_intf_t *intf, groute_route_t *route, groute_gw_t *gw, uint8_t reason)
{
	int i;
	groute_addr_t cb_addr;
	groute_callback_gw_desc_t cb_gw_desc[GROUTE_GW_NB];

	gw->find_network 		= 1;
	memcpy(gw->out_intf_name, intf->intf_name, GROUTE_INTF_NAME_SIZE);
	// collect all valid gateway in route
	for (i = 0; i < route->nb_gw; i++) {
		cb_gw_desc[i].find_network 	= route->gateway[i].find_network;
		cb_gw_desc[i].weight 		= route->gateway[i].weight;
		memcpy(cb_gw_desc[i].addr, route->gateway[i].addr, 16);
		memcpy(cb_gw_desc[i].out_intf_name, route->gateway[i].out_intf_name, GROUTE_INTF_NAME_SIZE);
	}

	// call route change callback function to kernel and user
	cb_addr.af_family 	= route->af_family;
	cb_addr.prefix_len 	= route->prefix;
	memcpy(cb_addr.address, route->dst_addr, 16);
	
	g_user_cb.route_change(reason, cb_addr, route->metric, route->nb_gw, cb_gw_desc);
	g_kernel_cb.route_change(reason, cb_addr, route->metric, route->nb_gw, cb_gw_desc);

	// link gateway to interface gw_node and update gateway information
	route->gateway_has_nw 	= 1;
	glist_del_init(&gw->gw_node);
	glist_add(&gw->gw_node, &intf->gw_node);
	return;
}

/*!
<pre>
	This private function is responsible to call user/kernel callback function when there is
	gateway can't find output network anymore
</pre>
	@param[in] intf groute_intf_t pointer point to interface
	@param[in] route groute_route_t pointer point to route
	@param[in] gw groute_gw_t pointer point to the gateway that missing output network 
	@param[in] reason to illustrated what configuration causing this result
	@author Tim Su
*/
static void __route_gw_dead(groute_intf_t *intf, groute_route_t *route, groute_gw_t *gw, uint8_t reason)
{
	int i, valid_gw = 0;
	groute_addr_t cb_addr;
	groute_callback_gw_desc_t cb_gw_desc[GROUTE_GW_NB];

	gw->find_network = 0;
	memset(gw->out_intf_name, 0, GROUTE_INTF_NAME_SIZE);
	// collect all valid gateway in route
	for (i = 0; i < route->nb_gw; i++) {
		if (route->gateway[i].find_network) {
			valid_gw++;
		}
		cb_gw_desc[i].find_network 	= route->gateway[i].find_network;
		cb_gw_desc[i].weight 		= route->gateway[i].weight;
		memcpy(cb_gw_desc[i].addr, route->gateway[i].addr, 16);
		memcpy(cb_gw_desc[i].out_intf_name, route->gateway[i].out_intf_name, GROUTE_INTF_NAME_SIZE);
	}

	// call route change callback function to kernel and user (ignore failed)
	cb_addr.af_family 	= route->af_family;
	cb_addr.prefix_len 	= route->prefix;
	memcpy(cb_addr.address, route->dst_addr, 16);
	
	g_user_cb.route_change(reason, cb_addr, route->metric, route->nb_gw, cb_gw_desc);
	g_kernel_cb.route_change(reason, cb_addr, route->metric, route->nb_gw, cb_gw_desc);

	// update new gateway status
	if (valid_gw == 0) {
		route->gateway_has_nw = 0;
	}

	glist_del_init(&gw->gw_node);
	glist_add(&gw->gw_node, &g_gw_idle_list);
	return;
}

/*!
<pre>
	This private function is responsible to make a interface polling all gateways not finding output network,
	and try to find the gateways should belongs to itself
</pre>
	@param[in] intf_ptr groute_intf_t pointer point to interface
	@param[in] family IPv4/IPv6 protocol
	@param[in] reason to illustrated what configuration causing this result
	@author Tim Su
*/
static void __intf_polling_idle_gw(groute_intf_t *intf_ptr, uint8_t family, uint8_t reason)
{
	groute_gw_t 	*gw_ptr, *temp_gw;
	if (!intf_ptr->link) {
		return;
	}
	// lookup all idle gw to check
	glist_for_each_entry_safe(gw_ptr, temp_gw, &g_gw_idle_list, gw_node) {
		if (family == AF_INET) {
			if (__output_nw_check(gw_ptr->addr, intf_ptr->ipv4_addr, intf_ptr->ipv4_prefix, family)) {
				__route_gw_alive(intf_ptr, gw_ptr->belongs_route, gw_ptr, reason);
			}
		} else {
			if (__output_nw_check(gw_ptr->addr, intf_ptr->ipv6_addr, intf_ptr->ipv6_prefix, family)) {
				__route_gw_alive(intf_ptr, gw_ptr->belongs_route, gw_ptr, reason);
			}
		}
	}

	return;
}

/*!
<pre>
	This private function is responsible to make a interface polling all gateways belongs to itself,
	and make it no longer belongs itself
</pre>
	@param[in] intf_ptr groute_intf_t pointer point to interface
	@param[in] family IPv4/IPv6 protocol
	@param[in] reason to illustrated what configuration causing this result
	@author Tim Su
*/
static void __intf_polling_linked_gw(groute_intf_t *intf_ptr, uint8_t family, uint8_t reason)
{	
	int ret;
	groute_intf_t 	*temp_intf_ptr;
	groute_gw_t 	*gw_ptr, *temp_gw;
	// lookup all linked gw of the interface to check
	glist_for_each_entry_safe(gw_ptr, temp_gw, &intf_ptr->gw_node, gw_node) {
		if (gw_ptr->family != family) {
			continue;
		}

		__route_gw_dead(intf_ptr, gw_ptr->belongs_route, gw_ptr, reason);

		// polling interface in used interface list to find other output network
		glist_for_each_entry(temp_intf_ptr, &g_intf_used_list, list_node) {
			if (!temp_intf_ptr->link) {
				continue;
			}
			ret = 0;
			if (family == AF_INET && temp_intf_ptr->ipv4_flag) {
				ret = __output_nw_check(gw_ptr->addr, temp_intf_ptr->ipv4_addr, temp_intf_ptr->ipv4_prefix, AF_INET);
			} else if (temp_intf_ptr->ipv6_flag) {
				ret = __output_nw_check(gw_ptr->addr, temp_intf_ptr->ipv6_addr, temp_intf_ptr->ipv6_prefix, AF_INET6);
			}

			// gateway find a new output network
			if (ret) {
				__route_gw_alive(temp_intf_ptr, gw_ptr->belongs_route, gw_ptr, reason);
			}
		}
	}

	return;
}

/*!
<pre>
	This private function is responsible to make a route polling all interface and try to make it's gateway
	finding output network
</pre>
	@param[in] route groute_route_t pointer point to route
	@author Tim Su
*/
static void __route_find_output_nw(groute_route_t *route)
{
	int i;
	groute_intf_t *intf_ptr;

	// lookup all gateway of the route
	for (i = 0; i < GROUTE_GW_NB; i++) {
		// lookup all interface to check output network
		glist_for_each_entry(intf_ptr, &g_intf_used_list, list_node) {
			if (!intf_ptr->use || !intf_ptr->link) {
				continue;
			}

			if (route->af_family == AF_INET) {
				if (!intf_ptr->ipv4_flag) {
					continue;
				}

				if (__output_nw_check(route->gateway[i].addr, intf_ptr->ipv4_addr, intf_ptr->ipv4_prefix, AF_INET)) {
					// match interface ipv4 address as output network
					route->gateway_has_nw = 1;
					route->gateway[i].find_network = 1;
					memcpy(route->gateway[i].out_intf_name, intf_ptr->intf_name, GROUTE_INTF_NAME_SIZE);
					glist_add(&route->gateway[i].gw_node, &intf_ptr->gw_node);
				}
			} else {
				if (!intf_ptr->ipv6_flag) {
					continue;
				}

				if (__output_nw_check(route->gateway[i].addr, intf_ptr->ipv6_addr, intf_ptr->ipv6_prefix, AF_INET6)) {
					// match interface ipv6 address as output network
					route->gateway_has_nw = 1;
					route->gateway[i].find_network = 1;
					memcpy(route->gateway[i].out_intf_name, intf_ptr->intf_name, GROUTE_INTF_NAME_SIZE);
					glist_add(&route->gateway[i].gw_node, &intf_ptr->gw_node);
				}
			}
		}

		// put all gateway which doesn't find output network to g_gw_idle_list
		if (route->gateway[i].use && !route->gateway[i].find_network) {
			glist_add(&route->gateway[i].gw_node, &g_gw_idle_list);
		}
	}

	return;
}

/*!
<pre>
	This private function is responsible to re-assigned global variable and 
	calculate new memory address of route and interface node
</pre>
	@return None
	@author Tim Su
*/
void __reconstruct_groute()
{
	int i, j;
	int64_t route_base_diff;

	route_base_diff = (int64_t)((uint64_t)g_route_shm - g_route_shm->base_addr);
	g_route_shm->base_addr = (uint64_t)g_route_shm;

	// global variable cant be arranged through route_base_diff (because it's not SHM)
	INIT_GLIST_HEAD(&g_route_free_list);
	INIT_GLIST_HEAD(&g_gw_idle_list);

	INIT_GLIST_HEAD(&g_intf_free_list);
	INIT_GLIST_HEAD(&g_intf_used_list);

	// arrange interface node
	for (i = 0; i < GROUTE_INTF_NB; i++) {
		if (g_intf_shm[i].use) {
			INIT_GLIST_HEAD(&g_intf_shm[i].list_node);
			glist_add(&g_intf_shm[i].list_node ,&g_intf_used_list);
		} else {
			INIT_GLIST_HEAD(&g_intf_shm[i].list_node);
			glist_add(&g_intf_shm[i].list_node ,&g_intf_free_list);
		}

		INIT_GLIST_HEAD(&g_intf_shm[i].gw_node);
	}

	// arrange route hash list
	for (i = 0; i < GROUTE_HASH_SIZE; i++) {
		g_route_shm->hash_list[i].prev = (void *)((int64_t)g_route_shm->hash_list[i].prev + route_base_diff);
		g_route_shm->hash_list[i].next = (void *)((int64_t)g_route_shm->hash_list[i].next + route_base_diff); 
	}

	// arrange route node
	for (i = 0; i < GROUTE_ROUTE_NB; i++) {
		if (g_route_shm->route_pool[i].use) {
			g_route_shm->route_pool[i].list_node.prev = (void *)((int64_t)g_route_shm->route_pool[i].list_node.prev + route_base_diff);
			g_route_shm->route_pool[i].list_node.next = (void *)((int64_t)g_route_shm->route_pool[i].list_node.next + route_base_diff);
		} else {
			INIT_GLIST_HEAD(&g_route_shm->route_pool[i].list_node);
			glist_add(&g_route_shm->route_pool[i].list_node, &g_route_free_list);
			for (j = 0; j < GROUTE_GW_NB; j++) {
				INIT_GLIST_HEAD(&g_route_shm->route_pool[i].gateway[j].gw_node);
			}
		}

		for (j = 0; j < GROUTE_GW_NB; j++) {
			INIT_GLIST_HEAD(&g_route_shm->route_pool[i].gateway[j].gw_node);
		}

		__route_find_output_nw(&g_route_shm->route_pool[i]);
	}

	return;
}

/*!
<pre>
	This private function is a backup callback function when user doesn't give valid parameter when initialization
</pre>
	@return Always GROUTE_CB_SUCCESS
	@author Tim Su
*/
int __fake_address_set(char *name, groute_addr_t addr)
{
	return GROUTE_CB_SUCCESS;
}
/*!
<pre>
	This private function is a backup callback function when user doesn't give valid parameter when initialization
</pre>
	@return Always GROUTE_CB_SUCCESS
	@author Tim Su
*/
int __fake_address_unset(char *name, groute_addr_t addr)
{
	return GROUTE_CB_SUCCESS;
}
/*!
<pre>
	This private function is a backup callback function when user doesn't give valid parameter when initialization
</pre>
	@return Always GROUTE_CB_SUCCESS
	@author Tim Su
*/
int __fake_link_set(char *name, uint8_t set)
{
	return GROUTE_CB_SUCCESS;
}
/*!
<pre>
	This private function is a backup callback function when user doesn't give valid parameter when initialization
</pre>
	@return Always GROUTE_CB_SUCCESS
	@author Tim Su
*/
int __fake_route_add(groute_addr_t dst, uint32_t metric, uint8_t nb_gw, groute_callback_gw_desc_t *gw_desc)
{
	return GROUTE_CB_SUCCESS;
}
/*!
<pre>
	This private function is a backup callback function when user doesn't give valid parameter when initialization
</pre>
	@return Always GROUTE_CB_SUCCESS
	@author Tim Su
*/
int __fake_route_del(groute_addr_t dst, uint32_t metric)
{
	return GROUTE_CB_SUCCESS;
}
/*!
<pre>
	This private function is a backup callback function when user doesn't give valid parameter when initialization
</pre>
	@return Always GROUTE_CB_SUCCESS
	@author Tim Su
*/
int __fake_route_change(groute_change_e reason, groute_addr_t dst, uint32_t metric, uint8_t nb_gw, groute_callback_gw_desc_t *gw_desc)
{
	return GROUTE_CB_SUCCESS;
}
// Public function
/*!
<pre>
	This function is responsible to get all memory resource and initilize it.
	It has to follow the spec define below:

	[v1.0-1] When Groute INIT, it will ask for call back function for each type for kernel and users pace, 
	if one of callback function is NULL, Grouting will return GROUTE_INIT_FAILED:
	Address_Set: set ip address of an interface
	Address_Unset: unset ip address of an interface
	Link_Set: Set admin link of an interface
	Route_Add: add a routing
	Route_Del: del a routing
	Route_Change: routing is changed when interface related config changed.

	[v1.0-2] When Groute INIT, it will return GROUTE_INIT_FAILED if cannot allocate resource.

</pre>
	@param[in] kernel groute_conf_cb_t kernel callback function
	@param[in] user groute_conf_cb_t user callback function
	@return GROUTE_INIT_FAILED or GROUTE_SUCCESS
	@author Tim Su
*/
int groute_init(groute_conf_cb_t *kernel, groute_conf_cb_t *user)
{
	int shm_ret_id;
	int shm_exist = 1;

	g_cb_valid = 1;

	if (kernel == NULL || user == NULL) {
		g_cb_valid = 0;
		// indicate callback to fake function
		g_kernel_cb.address_set 	= __fake_address_set;
		g_kernel_cb.address_unset 	= __fake_address_unset;
		g_kernel_cb.link_set 		= __fake_link_set;
		g_kernel_cb.route_add 		= __fake_route_add;
		g_kernel_cb.route_del 		= __fake_route_del;
		g_kernel_cb.route_change 	= __fake_route_change;

		g_user_cb.address_set 		= __fake_address_set;
		g_user_cb.address_unset 	= __fake_address_unset;
		g_user_cb.link_set 			= __fake_link_set;
		g_user_cb.route_add 		= __fake_route_add;
		g_user_cb.route_del 		= __fake_route_del;
		g_user_cb.route_change 		= __fake_route_change;
		goto NO_INPUT_ARG;
	}
	// pointer check
	if (kernel->address_set == NULL || kernel->address_unset == NULL || kernel->link_set == NULL ||
		kernel->route_add == NULL || kernel->route_del == NULL || kernel->route_change == NULL) {
		return GROUTE_INIT_FAILED;
	}

	if (user->address_set == NULL || user->address_unset == NULL || user->link_set == NULL ||
		user->route_add == NULL || user->route_del == NULL || user->route_change == NULL) {
		return GROUTE_INIT_FAILED;
	}
	
	// register callback function
	g_kernel_cb = *kernel;
	g_user_cb 	= *user;

NO_INPUT_ARG:
	// given shm pointer failed value first
	g_intf_shm 	= (groute_intf_t *)(-1);
	g_route_shm = (groute_route_shm_t *)(-1);

	// get INTF shm
	shm_ret_id 	= INJECT_FUNC(shmget, stub_shmget)(GROUTE_INTF_SHM_KEY, GROUTE_INTF_NB * sizeof(groute_intf_t), 0666);
	if (shm_ret_id == -1) {
		// first time get shm
		shm_exist  = 0;
		shm_ret_id = INJECT_FUNC(shmget, stub_shmget)(GROUTE_INTF_SHM_KEY, GROUTE_INTF_NB * sizeof(groute_intf_t), IPC_CREAT | 0666);
		if (shm_ret_id == -1) {
			return GROUTE_INIT_FAILED;
		}
	}

	// attach INTF shm
	g_intf_shm 	= (groute_intf_t *)INJECT_FUNC(shmat, stub_shmat)(shm_ret_id, NULL, 0);
	if (g_intf_shm == (groute_intf_t *)(-1)) {
		return GROUTE_INIT_FAILED;
	}

	// get ROUTE shm
	shm_ret_id 	= INJECT_FUNC(shmget, stub_shmget)(GROUTE_ROUTE_SHM_KEY, sizeof(groute_route_shm_t), 0666);
	if (shm_ret_id == -1) {
		// first time get shm
		shm_ret_id = INJECT_FUNC(shmget, stub_shmget)(GROUTE_ROUTE_SHM_KEY, sizeof(groute_route_shm_t), IPC_CREAT | 0666);
		if (shm_ret_id == -1) {
			return GROUTE_INIT_FAILED;
		}
	}

	// attach ROUTE shm
	g_route_shm 	= (groute_route_shm_t *)INJECT_FUNC(shmat, stub_shmat)(shm_ret_id, NULL, 0);
	if (g_route_shm == (groute_route_shm_t *)(-1)) {
		INJECT_FUNC(shmdt, stub_shmdt)(g_intf_shm);
		return GROUTE_INIT_FAILED;
	}

	// initialize lock
	memset(&g_route_shm->lock, 0, sizeof(uint8_t));

	if (!shm_exist) {
		__lock(&g_route_shm->lock);
		// initialize the pool
		memset(g_intf_shm, 0, GROUTE_INTF_NB * sizeof(groute_intf_t));
		__init_intf_pool();
		memset(&g_route_shm->base_addr, 0, sizeof(groute_route_shm_t) - 1);
		__init_route_pool();
		__unlock(&g_route_shm->lock);
	} else {
		__lock(&g_route_shm->lock);
		// reconstruct the groute status according to the information stored in SHM
		__reconstruct_groute();
		__unlock(&g_route_shm->lock);
	}
	return GROUTE_SUCCESS;
}

/*!
<pre>
	This function is responsible to clear all config stored in shared memory
</pre>
	@return GROUTE_SUCCESS
	@author Tim Su
*/
int groute_clear()
{
	__lock(&g_route_shm->lock);
	// initialize the pool
	memset(g_intf_shm, 0, GROUTE_INTF_NB * sizeof(groute_intf_t));
	__init_intf_pool();
	memset(&g_route_shm->base_addr, 0, sizeof(groute_route_shm_t) - 1);
	__init_route_pool();
	__unlock(&g_route_shm->lock);
	return GROUTE_SUCCESS;
}
/*!
<pre>
	This function is responsible to set interface address
	It has to follow the spec define below:

	[v1.0-3] When GRoute Set interface address, it would return GROUTE_ARG_FAILED, if following condition happened:

	AF_FAMILY != AF_INET && AF_FAMILY != AF_INET6
	AF_FAMILY = AF_INET && prefix_len > 32
	AF_FAMILY = AF_INET6 && prefix_len >128
	[v1.0-4] When Groute Set interface address, it would return GROUTE_ADDR_DUP if set address of same Network type twice.

	[v1.0-5] When Groute Set interface address, it would return GROUTE_ADDR_DUP if previous config have same address.

	[v1.0-6] in Groute Set interface address, it would try to execute user callback address_set first , it will return 
	GROUTE_FAILED if user callback function return GROUTE_CB_FAILED, or GRoute would execute user callback address_unset 
	when later get GROUTE_CB_FAILED after calling kernel address_set to make both side configuration sync.

	[v1.0-7] When Groute set interface address, GRoute should check all the routing which could not found output network 
	in previous setting, and execute Route_Change if those can find output network and finally return GROUTE_SUCCESS.

	[v1.0-8]in Groute Set interface address, Groute will execute address_set cb then Route_Change

	[v1.0-9]in Groute set interface address, it would return GROUTE_NO_RESOURCE if internal data storage is full.

</pre>
	@param[in] name pointer point to interface name
	@param[in] addr groute_addr_t format addresss information
	@return GROUTE_NO_RESOURCE,  GROUTE_ARG_FAILED, GROUTE_ADDR_DUP or GROUTE_SUCCESS
	@author Tim Su
*/
int groute_intf_addr_set(char *name, groute_addr_t addr)
{
	int ret;
	int intf_created = 0;
	groute_intf_t *intf_ptr;

	if (addr.af_family != AF_INET && addr.af_family != AF_INET6) {
		return GROUTE_ARG_FAILED;
	}

	if (addr.af_family == AF_INET && addr.prefix_len > 32) {
		return GROUTE_ARG_FAILED;
	}

	if (addr.af_family == AF_INET6 && addr.prefix_len > 128) {
		return GROUTE_ARG_FAILED;
	}

	__lock(&g_route_shm->lock);
	// check address duplicate in other interface
	glist_for_each_entry(intf_ptr, &g_intf_used_list, list_node) {
		if (addr.af_family == AF_INET && intf_ptr->ipv4_flag) {
			if (!memcmp(intf_ptr->ipv4_addr, addr.address, 4)) {
				__unlock(&g_route_shm->lock);
				return GROUTE_ADDR_DUP;
			}
		} else if (addr.af_family == AF_INET6 && intf_ptr->ipv6_flag) {
			if (!memcmp(intf_ptr->ipv6_addr, addr.address, 16)) {
				__unlock(&g_route_shm->lock);
				return GROUTE_ADDR_DUP;
			}
		}
	}

	// check interface is existent or not
	intf_ptr = __intf_lookup(name);

	if (intf_ptr == NULL) {
		intf_created = 1;
		intf_ptr = __intf_get(name);
		if (intf_ptr == NULL) {
			// run out of memeory
			__unlock(&g_route_shm->lock);
			return GROUTE_NO_RESOURCE;
		}
		// always set kernel link up when first time get intf entry
		g_kernel_cb.link_set(name, 1);
	} else if (addr.af_family == AF_INET && intf_ptr->ipv4_flag) {
		__unlock(&g_route_shm->lock);
		return GROUTE_ADDR_DUP;
	} else if (intf_ptr->ipv6_flag) {
		__unlock(&g_route_shm->lock);
		return GROUTE_ADDR_DUP;
	}

	// call user callback function
	ret = g_user_cb.address_set(name, addr);
	if (ret == GROUTE_CB_FAILED) {
		if (intf_created) {
			__intf_put(intf_ptr);
		}
		__unlock(&g_route_shm->lock);
		return GROUTE_FAILED;
	}

	// call kernel callback
	ret = g_kernel_cb.address_set(name, addr);
	if (ret == GROUTE_CB_FAILED) {
		g_user_cb.address_unset(name, addr);
		if (intf_created) {
			__intf_put(intf_ptr);
		}
		__unlock(&g_route_shm->lock);
		return GROUTE_FAILED;
	}

	__set_intf_address(intf_ptr, addr.af_family, addr.address, addr.prefix_len);
	__intf_polling_idle_gw(intf_ptr, addr.af_family, GROUTE_SET_ADDR);

	__unlock(&g_route_shm->lock);
	return GROUTE_SUCCESS;
}

/*!
<pre>
	This function is responsible to unset interface address
	It has to follow the spec define below:

	[v1.0-10] When GRoute unset interface address, it would return GROUTE_ARG_FAILED, 
	when AF_FAMILY != AF_INET and AF_FAMILY != AF_INET6

	[v1.0-11] When Groute unset interface address which did not config before, it would return GROUTE_ADDR_NOT_FOUND.

	[v1.0-12] When Groute unset interface address, GRoute should check all the routing which found output network 
	in previous setting, and execute Route_Change if those cannot find output network or find alternative network 
	and finally return GROUTE_SUCCESS.

	[v1.0-13]in Groute Unset interface address, Groute will execute Route_Change cb then address_unset.

</pre>
	@param[in] name pointer point to interface name
	@param[in] af_family IPv4/IPv6 protocol
	@return GROUTE_NO_RESOURCE,  GROUTE_ADDR_NOT_FOUND or GROUTE_SUCCESS
	@author Tim Su
*/
int groute_intf_addr_unset(char *name, uint8_t af_family)
{
	groute_intf_t *intf_ptr;
	groute_addr_t addr;

	if (af_family != AF_INET && af_family != AF_INET6) {
		return GROUTE_ARG_FAILED;
	}

	__lock(&g_route_shm->lock);
	// check interface is existent or not
	intf_ptr = __intf_lookup(name);
	if (intf_ptr == NULL) {
		__unlock(&g_route_shm->lock);
		return GROUTE_ADDR_NOT_FOUND;
	}

	// check corresponding address is ser or not
	if (af_family == AF_INET && !intf_ptr->ipv4_flag) {
		__unlock(&g_route_shm->lock);
		return GROUTE_ADDR_NOT_FOUND;
	} else if (af_family == AF_INET6 && !intf_ptr->ipv6_flag) {
		__unlock(&g_route_shm->lock);
		return GROUTE_ADDR_NOT_FOUND;
	}

	memset(&addr, 0, sizeof(groute_addr_t));
	addr.af_family = af_family;
	if (af_family == AF_INET) {
		addr.prefix_len = intf_ptr->ipv4_prefix;
		memcpy(addr.address, intf_ptr->ipv4_addr, 4);
	} else {
		addr.prefix_len = intf_ptr->ipv6_prefix;
		memcpy(addr.address, intf_ptr->ipv6_addr, 16);
	}

	__unset_intf_address(intf_ptr, af_family);
	__intf_polling_linked_gw(intf_ptr, af_family, GROUTE_UNSET_ADDR);

	// calling kernel and user callback function
	g_user_cb.address_unset(name, addr);
	g_kernel_cb.address_unset(name, addr);
	__unlock(&g_route_shm->lock);
	return GROUTE_SUCCESS;
}

/*!
<pre>
	This function is responsible to set/uset interface administrate up/down
	It has to follow the spec define below:

	[v1.0-14] In Groute Link_set, if previous setting equal to request, Groute would return GROUTE_SUCCESS 
	without executing callback function.

	[v1.0-15] When Groute link_set is down , GRoute should check all the routing which found output network in 
	previous setting, and execute Route_Change if those cannot find output network or find alternative network 
	and finally return GROUTE_SUCCESS.

	[v1.0-16]in Groute link_set down, Groute will execute Route_Change cb then link_set.

	[v1.0-17] in Groute link_set down, after execute link_set cb, if interface contains IPV6 address, Groute would 
	execute kernel address_set of IPV6 again.

	[v1.0-18] When Groute link_set is up , GRoute should check all the routing which cannot found output network in 
	previous setting, and execute Route_Change if those find output network and finally return GROUTE_SUCCESS.

	[v1.0-19]in Groute link_set up, Groute will execute link_set cb then Route_Change.

	[v1.0-20]in Groute set link , it would return GROUTE_NO_RESOURCE if internal data storage is full.

</pre>
	@param[in] name pointer point to interface name
	@param[in] set interface requiring link status
	@return GROUTE_NO_RESOURCE, or GROUTE_SUCCESS
	@author Tim Su
*/
int groute_intf_link_set(char *name, uint8_t set)
{
	groute_addr_t cb_addr;
	groute_intf_t *intf_ptr;

	__lock(&g_route_shm->lock);
	// check interface is existent or not
	intf_ptr = __intf_lookup(name);
	if (intf_ptr == NULL) {
		intf_ptr = __intf_get(name);
		if (intf_ptr == NULL) {
			// run out of memeory
			__unlock(&g_route_shm->lock);
			return GROUTE_NO_RESOURCE;
		}
	}

	// check link is different from intf_ptr or not
	if (intf_ptr->link == set) {
		__unlock(&g_route_shm->lock);
		return GROUTE_SUCCESS;
	}

	intf_ptr->link = set;
	if (set) {
		// set link callback function first
		g_user_cb.link_set(name, 1);
		g_kernel_cb.link_set(name, 1);
		// update ipv4/ipv6 route back later
		__intf_polling_idle_gw(intf_ptr, AF_INET, GROUTE_SET_LINK);
		__intf_polling_idle_gw(intf_ptr, AF_INET6, GROUTE_SET_LINK);
	} else {
		// delete corresponding both ipv4/ipv6 reoute first
		__intf_polling_linked_gw(intf_ptr, AF_INET, GROUTE_SET_LINK);
		__intf_polling_linked_gw(intf_ptr, AF_INET6, GROUTE_SET_LINK);
		// set link callback function later
		g_user_cb.link_set(name, 0);
		g_kernel_cb.link_set(name, 0);
		// kernel will remove ipv6 address if set before
		// so groute have to add it back and it should always success
		// because this ipv6 address has been added before
		if (intf_ptr->ipv6_flag) {
			cb_addr.af_family 	= AF_INET6;
			cb_addr.prefix_len 	= intf_ptr->ipv6_prefix;
			memcpy(cb_addr.address, intf_ptr->ipv6_addr, 16);
			g_kernel_cb.address_set(name, cb_addr);
		}
	}

	__unlock(&g_route_shm->lock);
	return GROUTE_SUCCESS;
}

/*!
<pre>
	This function is responsible to add route
	It has to follow the spec define below:

	[v1.0-21] When GRoute Add_route, it would return GROUTE_ARG_FAILED, if following condition happened:

	AF_FAMILY != AF_INET && AF_FAMILY != AF_INET6
	AF_FAMILY = AF_INET && prefix_len > 32
	AF_FAMILY = AF_INET6 && prefix_len >128
	nb_gw value is larger than GROUTE_GW_NB
	[v1.0-22] When GRoute Add_route, it would return GROUTE_ROUTE_DUP when meet 2 routing with same Destination, 
	Prefix and metric.

	[v1.0-23]in Groute add route , it would return GROUTE_NO_RESOURCE if internal data storage is full.

	[v1.0-24] in Groute add route, it would execute route_add bothside. Thefind_network in gateway descriptor represent 
	Groute find a output network or not.

	[v1.0-25] in Groute add_route , it would try to execute user callback route_add first , it will return GROUTE_FAILED 
	if user callback function return GROUTE_CB_FAILED, or GRoute would execute user callback route_del when later get 
	GROUTE_CB_FAILED after calling kernel route_add to make both side configuration sync.

</pre>
	@param[in] dst groute_addt_t format destination address information
	@param[in] metric route metric
	@param[in] nb_gw configuring gateway number
	@param[in] gw_desc groute_gw_desc_t format gateway information
	@return GROUTE_ARG_FAILED, GROUTE_ROUTE_DUP, GROUTE_NO_RESOURCE, GROUTE_FAILED, or GROUTE_SUCCESS
	@author Tim Su
*/
int groute_route_add(groute_addr_t dst, uint32_t metric, uint8_t nb_gw, groute_gw_desc_t *gw_desc)
{
	int i, ret;
	groute_route_t *route_ptr;
	groute_callback_gw_desc_t cb_gw_desc[GROUTE_GW_NB];

	if (dst.af_family != AF_INET && dst.af_family != AF_INET6) {
		return GROUTE_ARG_FAILED;
	}

	if (dst.af_family == AF_INET && dst.prefix_len > 32) {
		return GROUTE_ARG_FAILED;
	}

	if (dst.af_family == AF_INET6 && dst.prefix_len > 128) {
		return GROUTE_ARG_FAILED;
	}

	if (nb_gw > GROUTE_GW_NB) {
		return GROUTE_ARG_FAILED;
	}

    if (dst.af_family == AF_INET) {
        memset(&dst.address[4], 0, 12);
    }

	__lock(&g_route_shm->lock);
	// check route duplicate
	if (__route_lookup(dst.address, dst.prefix_len, metric)) {
		__unlock(&g_route_shm->lock);
		return GROUTE_ROUTE_DUP;
	}

	// get route node from pool
	route_ptr = __route_get(dst.address, dst.prefix_len, metric, dst.af_family, nb_gw);
	if (route_ptr == NULL) {
		__unlock(&g_route_shm->lock);
		return GROUTE_NO_RESOURCE;
	}

	// assign gateway information
	__assign_gw(route_ptr, nb_gw, gw_desc);
	// add route to hash
	__route_add_hash(route_ptr);
	// check gateway has output network or not
	__route_find_output_nw(route_ptr);

	for (i = 0; i < nb_gw; i++) {
		memcpy(cb_gw_desc[i].out_intf_name, route_ptr->gateway[i].out_intf_name, GROUTE_INTF_NAME_SIZE);
		memcpy(cb_gw_desc[i].addr, route_ptr->gateway[i].addr, 16);
		cb_gw_desc[i].find_network 	= route_ptr->gateway[i].find_network;
		cb_gw_desc[i].weight 		= route_ptr->gateway[i].weight;
	}

	// call user callback function after
	ret = g_user_cb.route_add(dst, metric, nb_gw, cb_gw_desc);
	if (ret == GROUTE_CB_FAILED) {
		__route_put(route_ptr);
		__unlock(&g_route_shm->lock);
		return GROUTE_FAILED;
	}

	// call kernel callback function first
	ret = g_kernel_cb.route_add(dst, metric, nb_gw, cb_gw_desc);
	if (ret == GROUTE_CB_FAILED) {
		g_user_cb.route_del(dst, metric);
		__route_put(route_ptr);
		__unlock(&g_route_shm->lock);
		return GROUTE_FAILED;
	}

	__unlock(&g_route_shm->lock);
	return GROUTE_SUCCESS;
}

/*!
<pre>
	This function is responsible to del route
	It has to follow the spec define below:

	[v1.0-26] When GRoute del_route, it would return GROUTE_ARG_FAILED, if following condition happened:

	AF_FAMILY != AF_INET && AF_FAMILY != AF_INET6
	AF_FAMILY = AF_INET && prefix_len > 32
	AF_FAMILY = AF_INET6 && prefix_len >128
	[v1.0-27] in Groute del_rotue, it would return GROUTE_ROUTE_NOT_FOUND if cannot found routing with same 
	Destination, prefix, and metric

	[v1.0-28] in Groute del_route , it would try to execute callback function of route_del with user callback first then 
	kernel callback without checking return value, and return GROUTE_SUCCESS

	[v1.0-29] in Groute del_route, it would call kernel callback function specific times according to gateway number when 
	this route is IPv6 ECMP route.

</pre>
	@param[in] dst groute_addt_t format destination address information
	@param[in] metric route metric
	@return GROUTE_ARG_FAILED, GROUTE_ROUTE_NOT_FOUND, or GROUTE_SUCCESS
	@author Tim Su
*/
int groute_route_del(groute_addr_t dst, uint32_t metric)
{
	int i;
	groute_route_t *route_ptr;

	if (dst.af_family != AF_INET && dst.af_family != AF_INET6) {
		return GROUTE_ARG_FAILED;
	}

	if (dst.af_family == AF_INET && dst.prefix_len > 32) {
		return GROUTE_ARG_FAILED;
	}

	if (dst.af_family == AF_INET6 && dst.prefix_len > 128) {
		return GROUTE_ARG_FAILED;
	}
    
    if (dst.af_family == AF_INET) {
        memset(&dst.address[4], 0, 12);
    }

	__lock(&g_route_shm->lock);
	// check route existent
	route_ptr = __route_lookup(dst.address, dst.prefix_len, metric);
	if (route_ptr == NULL) {
		__unlock(&g_route_shm->lock);
		return GROUTE_ROUTE_NOT_FOUND;
	}

	g_user_cb.route_del(dst, metric);


	if (dst.af_family == AF_INET6 && route_ptr->nb_gw > 1) {
		// kernel add ipv6 ecmp route as static routes
		for (i = 0; i < route_ptr->nb_gw; i++) {
			g_kernel_cb.route_del(dst, metric);
		}
	} else {
		g_kernel_cb.route_del(dst, metric);
	}

	__route_put(route_ptr);
	__unlock(&g_route_shm->lock);
	return GROUTE_SUCCESS;
}


/*!
<pre>
	This function is responsible to make other process get all interface and route configurations in Groute
	It has to follow the spec define below:

	[v1.0-30] Groute should provide API for reloading current config to other process. It should equipped method 
	for IPC for loading config. When Groute reload, it would loading configuration and execute new_intf with all 
	interface config then new_route with all routing config.

	[v1.0-31] When Groute cannot get config from IPC, no callback function would be executed.

	[v1.0-32] All the given callback function should not be NULL, or no callback function would be executed.

</pre>
	@param[in] cb callback function prepared in groute_reload_cb_t format
	@return GROUTE_SUCCESS
	@author Tim Su
*/
int groute_reload(groute_reload_cb_t cb)
{
	// this function is prepared for the user process
	int i, j;
	int shm_ret_id, addr_nb, gw_nb = 0;
	int64_t base_diff;
	groute_addr_t address[2];
	groute_route_t *route_ptr;
	groute_callback_gw_desc_t gw_desc[GROUTE_GW_NB];

	// pointer check
	if (cb.new_intf == NULL || cb.new_route == NULL) {
		return GROUTE_SUCCESS;
	}

	// get INTF shm
	shm_ret_id 	= INJECT_FUNC(shmget, stub_shmget)(GROUTE_INTF_SHM_KEY, GROUTE_INTF_NB * sizeof(groute_intf_t), 0666);
	if (shm_ret_id == -1) {
		return GROUTE_SUCCESS;
	}

	// attach INTF shm
	g_reload_dump_intf_shm 	= (groute_intf_t *)INJECT_FUNC(shmat, stub_shmat)(shm_ret_id, NULL, 0);
	if (g_reload_dump_intf_shm == (groute_intf_t *)(-1)) {
		return GROUTE_SUCCESS;
	}
	

	// get ROUTE shm
	shm_ret_id 	= INJECT_FUNC(shmget, stub_shmget)(GROUTE_ROUTE_SHM_KEY, sizeof(groute_route_shm_t), 0666);
	if (shm_ret_id == -1) {
		return GROUTE_SUCCESS;
	}

	// attach ROUTE shm
	g_reload_dump_route_shm 	= (groute_route_shm_t *)INJECT_FUNC(shmat, stub_shmat)(shm_ret_id, NULL, 0);
	if (g_reload_dump_route_shm == (groute_route_shm_t *)(-1)) {
		return GROUTE_SUCCESS;
	}

	__lock(&g_reload_dump_route_shm->lock);

	// !!! calculate base address differential value !!!
	base_diff = (int64_t)((uint64_t)g_reload_dump_route_shm - (g_reload_dump_route_shm->base_addr));

	// polling all interface to call reload callback function
	for (i = 0; i < GROUTE_INTF_NB; i++) {
		if (!g_reload_dump_intf_shm[i].use) {
			continue;
		}

		addr_nb = 0;

		if (g_reload_dump_intf_shm[i].ipv4_flag) {
			address[addr_nb].af_family 	= AF_INET;
			address[addr_nb].prefix_len = g_reload_dump_intf_shm[i].ipv4_prefix;
			memcpy(address[addr_nb].address, g_reload_dump_intf_shm[i].ipv4_addr, 4);
			addr_nb++;
		}

		if (g_reload_dump_intf_shm[i].ipv6_flag) {
			address[addr_nb].af_family 	= AF_INET6;
			address[addr_nb].prefix_len = g_reload_dump_intf_shm[i].ipv6_prefix;
			memcpy(address[addr_nb].address, g_reload_dump_intf_shm[i].ipv6_addr, 16);
			addr_nb++;
		}

		// reload interface
		cb.new_intf(g_reload_dump_intf_shm[i].intf_name, g_reload_dump_intf_shm[i].link, addr_nb, address);
	}

	// polling all route in hash list to call reload callback function
	for (i = 0; i < GROUTE_HASH_SIZE; i++) {
		// check hash list is empty or not
		if ((int64_t)g_reload_dump_route_shm->hash_list[i].next == ((int64_t)&g_reload_dump_route_shm->hash_list[i] - base_diff)) {
			continue;
		}

		do {
			route_ptr = glist_entry((void *)((int64_t)g_reload_dump_route_shm->hash_list[i].next + base_diff), groute_route_t, list_node);

			// assign route address
			address[0].af_family 	= route_ptr->af_family;
			address[0].prefix_len 	= route_ptr->prefix;
			memcpy(address[0].address, route_ptr->dst_addr, 16);

			// assign callback gateway description
			for (j = 0; j < GROUTE_GW_NB; j++) {
				if (route_ptr->gateway[j].use) {
					gw_desc[gw_nb].find_network = route_ptr->gateway[j].find_network;
					gw_desc[gw_nb].weight 		= route_ptr->gateway[j].weight;
					memcpy(gw_desc[gw_nb].out_intf_name, route_ptr->gateway[j].out_intf_name, GROUTE_INTF_NAME_SIZE);
					memcpy(gw_desc[gw_nb].addr, route_ptr->gateway[j].addr, 16);
					gw_nb++;
				}
			}
			// call callback function
			cb.new_route(address[0], route_ptr->metric, gw_nb, gw_desc);
			gw_nb = 0;
		} while ((int64_t)route_ptr->list_node.next != (int64_t)&g_reload_dump_route_shm->hash_list[i] - base_diff);
	}

	__unlock(&g_reload_dump_route_shm->lock);
	INJECT_FUNC(shmdt, stub_shmdt)(g_reload_dump_intf_shm);
	INJECT_FUNC(shmdt, stub_shmdt)(g_reload_dump_route_shm);
	return GROUTE_SUCCESS;
}

/*!
<pre>
	This function is responsible to dump all interface and route configurations in Groute to file pointer
	It has to follow the spec define below:

	[v1.0-33] Groute should provide API for dump current config to other process. It should equipped method for IPC for loading config. When Groute dump, it would printf configuration to the file pointer.

	[v1.0-34] When Groute cannot get config from IPC, no config would be dump.

	[v1.0-35] The given file pointer should not be NULL, or no config would be dump.

</pre>
	@param[in] fp file pointer to print
	@return GROUTE_SUCCESS
	@author Tim Su
*/
int groute_dump(FILE *fp)
{
	// this function is prepared for the user process
	int i, j;
	int shm_ret_id;
	int64_t base_diff;
	groute_route_t *route_ptr;

	// check file pointer
	if (fp == NULL) {
		return GROUTE_SUCCESS;
	}

	// get INTF shm
	shm_ret_id 	= INJECT_FUNC(shmget, stub_shmget)(GROUTE_INTF_SHM_KEY, GROUTE_INTF_NB * sizeof(groute_intf_t), 0666);
	if (shm_ret_id == -1) {
		return GROUTE_SUCCESS;
	}

	// attach INTF shm
	g_reload_dump_intf_shm 	= (groute_intf_t *)INJECT_FUNC(shmat, stub_shmat)(shm_ret_id, NULL, 0);
	if (g_reload_dump_intf_shm == (groute_intf_t *)(-1)) {
		return GROUTE_SUCCESS;
	}
	

	// get ROUTE shm
	shm_ret_id 	= INJECT_FUNC(shmget, stub_shmget)(GROUTE_ROUTE_SHM_KEY, sizeof(groute_route_shm_t), 0666);
	if (shm_ret_id == -1) {
		return GROUTE_SUCCESS;
	}

	// attach ROUTE shm
	g_reload_dump_route_shm 	= (groute_route_shm_t *)INJECT_FUNC(shmat, stub_shmat)(shm_ret_id, NULL, 0);
	if (g_reload_dump_route_shm == (groute_route_shm_t *)(-1)) {
		return GROUTE_SUCCESS;
	}

	__lock(&g_reload_dump_route_shm->lock);

	// !!! calculate base address differential value !!!
	base_diff = (int64_t)((uint64_t)g_reload_dump_route_shm - (g_reload_dump_route_shm->base_addr));
	fprintf(fp, "==========================================================================\n");
	fprintf(fp, "GROUTE maintain information:\n");
	// polling all interface to call reload callback function
	for (i = 0; i < GROUTE_INTF_NB; i++) {
		if (!g_reload_dump_intf_shm[i].use) {
			continue;
		}

		fprintf(fp, "Interface name : %s\n", g_reload_dump_intf_shm[i].intf_name);
		fprintf(fp, "Admin link\t	: %d\n", g_reload_dump_intf_shm[i].link);
		if (g_reload_dump_intf_shm[i].ipv4_flag) {
			fprintf(fp, "Ipv4 address 	: %d.%d.%d.%d/%d\n", g_reload_dump_intf_shm[i].ipv4_addr[0],
														 	 g_reload_dump_intf_shm[i].ipv4_addr[1],
														 	 g_reload_dump_intf_shm[i].ipv4_addr[2],
														 	 g_reload_dump_intf_shm[i].ipv4_addr[3],
														 	 g_reload_dump_intf_shm[i].ipv4_prefix);
		}

		if (g_reload_dump_intf_shm[i].ipv6_flag) {
			fprintf(fp, "Ipv6 address 	: %02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X/%d\n",
									g_reload_dump_intf_shm[i].ipv6_addr[0], g_reload_dump_intf_shm[i].ipv6_addr[1], g_reload_dump_intf_shm[i].ipv6_addr[2],
									g_reload_dump_intf_shm[i].ipv6_addr[3], g_reload_dump_intf_shm[i].ipv6_addr[4], g_reload_dump_intf_shm[i].ipv6_addr[5],
									g_reload_dump_intf_shm[i].ipv6_addr[6], g_reload_dump_intf_shm[i].ipv6_addr[7], g_reload_dump_intf_shm[i].ipv6_addr[8],
									g_reload_dump_intf_shm[i].ipv6_addr[9], g_reload_dump_intf_shm[i].ipv6_addr[10], g_reload_dump_intf_shm[i].ipv6_addr[11],
									g_reload_dump_intf_shm[i].ipv6_addr[12], g_reload_dump_intf_shm[i].ipv6_addr[13], g_reload_dump_intf_shm[i].ipv6_addr[14],
									g_reload_dump_intf_shm[i].ipv6_addr[15], g_reload_dump_intf_shm[i].ipv6_prefix);										 	
		}
		fprintf(fp, "\n\n");
	}
	fprintf(fp, "==========================================================================\n");
		// polling all route in hash list to call reload callback function
	for (i = 0; i < GROUTE_HASH_SIZE; i++) {
		// check hash list is empty or not
		if ((int64_t)g_reload_dump_route_shm->hash_list[i].next == ((int64_t)&g_reload_dump_route_shm->hash_list[i] - base_diff)) {
			continue;
		}

		do {
			route_ptr = glist_entry((void *)((int64_t)g_reload_dump_route_shm->hash_list[i].next + base_diff), groute_route_t, list_node);
			if (route_ptr->af_family == AF_INET) {
				fprintf(fp, "Route address 	: %d.%d.%d.%d/%d\n",route_ptr->dst_addr[0],
																route_ptr->dst_addr[1],
																route_ptr->dst_addr[2],
																route_ptr->dst_addr[3],
																route_ptr->prefix);
			} else {
				fprintf(fp, "Route address 	: %02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X/%d\n",
										route_ptr->dst_addr[0], route_ptr->dst_addr[1], route_ptr->dst_addr[2],
										route_ptr->dst_addr[3], route_ptr->dst_addr[4], route_ptr->dst_addr[5],
										route_ptr->dst_addr[6], route_ptr->dst_addr[7], route_ptr->dst_addr[8],
										route_ptr->dst_addr[9], route_ptr->dst_addr[10], route_ptr->dst_addr[11],
										route_ptr->dst_addr[12], route_ptr->dst_addr[13], route_ptr->dst_addr[14],
										route_ptr->dst_addr[15], route_ptr->prefix);
			}
			fprintf(fp, "metric			: %d\n", route_ptr->metric);
			fprintf(fp, "ecmp_flag 		: %d\n", route_ptr->ecmp_flag);
			fprintf(fp, "nb_gw			: %d\n", route_ptr->nb_gw);
			fprintf(fp, "gateway_has_nw	: %d\n", route_ptr->gateway_has_nw);

			// assign callback gateway description
			for (j = 0; j < route_ptr->nb_gw; j++) {
				if (route_ptr->gateway[j].family == AF_INET) {
				fprintf(fp, "\tGateway address : %d.%d.%d.%d\n",route_ptr->gateway[j].addr[0],
																 route_ptr->gateway[j].addr[1],
																 route_ptr->gateway[j].addr[2],
																 route_ptr->gateway[j].addr[3]);
				} else {
				fprintf(fp, "\tGateway address : %02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X\n",
						route_ptr->gateway[j].addr[0], route_ptr->gateway[j].addr[1], route_ptr->gateway[j].addr[2], route_ptr->gateway[j].addr[3], 
						route_ptr->gateway[j].addr[4], route_ptr->gateway[j].addr[5], route_ptr->gateway[j].addr[6], route_ptr->gateway[j].addr[7], 
						route_ptr->gateway[j].addr[8], route_ptr->gateway[j].addr[9], route_ptr->gateway[j].addr[10], route_ptr->gateway[j].addr[11],
						route_ptr->gateway[j].addr[12], route_ptr->gateway[j].addr[13], route_ptr->gateway[j].addr[14], route_ptr->gateway[j].addr[15]);
				}

				fprintf(fp, "\tweight 			: %d\n", route_ptr->gateway[j].weight);
				fprintf(fp, "\tfind_network 	: %d\n", route_ptr->gateway[j].find_network);
				fprintf(fp, "\toutput interface : %s\n", route_ptr->gateway[j].out_intf_name);
			}
		} while ((int64_t)route_ptr->list_node.next != (int64_t)&g_reload_dump_route_shm->hash_list[i] - base_diff);
	}
	fprintf(fp, "==========================================================================\n");
	__unlock(&g_reload_dump_route_shm->lock);
	INJECT_FUNC(shmdt, stub_shmdt)(g_reload_dump_intf_shm);
	INJECT_FUNC(shmdt, stub_shmdt)(g_reload_dump_route_shm);
	return GROUTE_SUCCESS;
}

#ifdef GROUTE_UNIT_TEST
/*!
<pre>
	This function is responsible to remove all interface and route configurations in Groute to make
	Groute back to the statu that just after initialization
</pre>
	@author Tim Su
*/
void groute_destroy ()
{
	// it's noticeed that groute_init() and groute_destory() should be called by same process
	// claer registered callback function
	memset(&g_kernel_cb, 0, sizeof(groute_conf_cb_t));
	memset(&g_user_cb, 0, sizeof(groute_conf_cb_t));

	if (g_intf_shm != (groute_intf_t *)(-1)) {
		// initialize the pool
		memset(g_intf_shm, 0, GROUTE_INTF_NB * sizeof(groute_intf_t));
		__init_intf_pool();
		INJECT_FUNC(shmdt, stub_shmdt)(g_intf_shm);
	}
	
	if (g_route_shm != (groute_route_shm_t *)(-1)) {
		// initialize lock
		memset(&g_route_shm->lock, 0, sizeof(uint8_t));
		usleep(1);
		__lock(&g_route_shm->lock);
		// initialize the pool
		memset(&g_route_shm->base_addr, 0, sizeof(groute_route_shm_t) - 1);
		__init_route_pool();
		__unlock(&g_route_shm->lock);
		INJECT_FUNC(shmdt, stub_shmdt)(g_route_shm);
	}
	return;
}
#endif
