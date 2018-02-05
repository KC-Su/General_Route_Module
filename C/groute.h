/*!
	@mainpage

	@section Introduction GRoute Synopsis
<pre>
	GRoute is a library used for configuration of interface and routing between user space and Linux.
	In some kernel, linux for example, routing cannot be configured due to cannot find output network.
	So GRoute will be response for integrating related config and notify user space and kernel, useage 
	of Groute should be the same process, except Reload and Dump.
</pre>
	
	@section Link Link to the source code file
	<src/groute.h><br>
	<src/groute.c><br>
	
    @file
    \brief Definition of GRoute data structure, Return value and Public API
*/

#ifndef GROUTE_H
#define GROUTE_H
#include <stdio.h>
#include <stdint.h>

#define GROUTE_SUCCESS 				(0)		//!< Return when operation is successful
#define GROUTE_FAILED				(-1)	//!< Return when operation is not successful
#define GROUTE_INIT_FAILED 			(-2)	//!< Return when any error happened in initialization process
#define GROUTE_NO_RESOURCE 			(-3)	//!< Return when memory pool is empty
#define GROUTE_ADDR_DUP 			(-4)	//!< Return when interface address value is not allowed to be added
#define GROUTE_ADDR_NOT_FOUND 		(-5)	//!< Return when interface address has not been configured before
#define GROUTE_ROUTE_DUP 			(-6)	//!< Return when route information is not allowed to be added
#define GROUTE_ROUTE_NOT_FOUND 		(-7)	//!< Return when route information has not been configured before
#define GROUTE_ARG_FAILED 			(-8)	//!< Return when meet any arguments format error
#define GROUTE_NOT_IMPLEMENT 		(-9)	//!< Return when Groute API just has prototype

#define GROUTE_CB_SUCCESS 			(0)		//!< Kernel/User callback function return value to notify Groute API
#define GROUTE_CB_FAILED			(-1)	//!< Kernel/User callback function return value to notifu Groute API

// Geneie RouTe shm key prefix ASCII: G(0x47) R(0x52) T(0x54)
#define GROUTE_INTF_SHM_KEY 		(0x47525400)	//!< GRoute API using SHM Key for Interface memory pool
#define GROUTE_ROUTE_SHM_KEY 		(0x47525401)	//!< GRoute API using SHM Key for Route memory pool

#ifdef GROUTE_UNIT_TEST
#define GROUTE_INTF_NAME_SIZE 		(32)		
#define GROUTE_HASH_SIZE 			(256)		
#define GROUTE_INTF_NB			 	(3)		
#define GROUTE_ROUTE_NB				(3)	
#define GROUTE_GW_NB 				(2)			
#else
#define GROUTE_INTF_NAME_SIZE 		(32)		//!< Allowing maximun interface name sizing
#define GROUTE_HASH_SIZE 			(256)		//!< Hash list sizing 
#define GROUTE_INTF_NB			 	(4099)		//!< Interface pool sizing
#define GROUTE_ROUTE_NB				(128*1024)	//!< Route pool sizing
#define GROUTE_GW_NB 				(8)			//!< Allowing maximun gateway number of a ECMP route
#endif

/*!
    \brief  A gateway infortation that GRoute needed
	
	When adding a route to GRoute, user have to prepare an array of gateway information as argument.
*/
typedef struct {
    uint8_t     addr[16];	//!< IPv4 and IPv6 shared 
    uint8_t     weight;		//!< Valid for ECMP route, or zero to static route
}groute_gw_desc_t;


/*!
	\brief A gateway information that Groute giving to user/kernel callback function

	When Groute calling user/kernel route_add/route_change callback function, it will also giving gateway information.
	It has find_network flag to make user/kernel know this gateway has output network or not, out_intf_name will be
	valid if find_network flag is set, or not valid when find_network flag is not set.
*/
typedef struct {
	uint8_t     addr[16];								//!< gateway address, IPv4/IPv6 shared
    uint8_t     weight;									//!< Valid for ECMP, or zero for static route
 	uint8_t 	find_network;							//!< to mark that this gateway has output network or not
    uint8_t 	out_intf_name[GROUTE_INTF_NAME_SIZE];	//!< Valid when find_network flag is set
}groute_callback_gw_desc_t;


/*!
	\brief Can be used to represent interface address or route destination address for Groute

	When set/unset interface address or route add/del/change, Groute will need this data structure
	to parsing the address information.
*/
typedef struct {
	uint8_t af_family;		//!< AF_INET or AF_INET6
	uint8_t address[16];	//!< Address that IPv4/IPv6 shared
	uint8_t prefix_len;		//!< Address mask length
}groute_addr_t;


/*!
	\brief The reason why Groute calling  route_change

	It's inforamtion to make kernel/user route_change callback function to know why route is changed.
*/
typedef enum{
	GROUTE_SET_ADDR,
	GROUTE_UNSET_ADDR,
	GROUTE_SET_LINK,
	GROUTE_UNSET_LINK,
}groute_change_e;


/*!
	\brief Callback functions that user and kernel have to prepare.

	Groute will passing interface and route configuration to user/kernel by using these callback function.
*/
typedef struct {
	int (*address_set)(char *name, groute_addr_t addr);		//!< Calling when an interface address is added 
	int (*address_unset)(char *name, groute_addr_t addr);	//!< Calling when an interface address is deleted
	int (*link_set)(char *name, uint8_t set);				//!< Calling when an interface was configured admin up/down
	int (*route_add)(groute_addr_t dst, uint32_t metric, uint8_t nb_gw, groute_callback_gw_desc_t *gw_desc); //!< Calling when a route is added
	int (*route_del)(groute_addr_t dst, uint32_t metric);	//!< Calling when a route is deleted
	int (*route_change)(groute_change_e reason, groute_addr_t dst, uint32_t metric, uint8_t nb_gw, groute_callback_gw_desc_t *gw_desc); //!< Calling when any gateway of a route status change in Groute
} groute_conf_cb_t;


/*!
	\brief Callback function to make other process get all Groute interface and route configuration

	Groute need these callback function to giving all interface and route configuration
*/
typedef struct {
	void (*new_intf)(char *name, uint8_t admin_link, uint8_t nb_addr, groute_addr_t *addr);
	void (*new_route)(groute_addr_t dst, uint32_t metric, uint8_t nb_gw, groute_callback_gw_desc_t *gw_desc);
} groute_reload_cb_t;

///////////// API  ////////////
int groute_init(groute_conf_cb_t *kenerl, groute_conf_cb_t *user);
int groute_clear();
int groute_intf_addr_set(char *name, groute_addr_t addr);
int groute_intf_addr_unset(char *name, uint8_t af_family);
int groute_intf_link_set(char *name, uint8_t set);
int groute_route_add(groute_addr_t dst, uint32_t metric, uint8_t nb_gw, groute_gw_desc_t *gw_desc);
int groute_route_del(groute_addr_t dst, uint32_t metric);
int groute_reload(groute_reload_cb_t cb);
int groute_dump(FILE *fp);

#ifdef GROUTE_UNIT_TEST
void groute_destroy();
#endif //GROUTE_UNIT_TEST

#endif //GROUTE_H
