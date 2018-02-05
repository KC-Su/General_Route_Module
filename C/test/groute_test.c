#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include "groute_test.h"
#include "groute.h"
#include <CUnit/CUnit.h>      //ASSERT definitions, test management.
#include <CUnit/Automated.h>  //Automated interface with xml output.
#include <CUnit/Basic.h>      //Basic interface with console output.
#include <CUnit/Console.h>	  //Console output

#define TEST_BASIC

#define GROUTE_TEST_S1      (1)
#define GROUTE_TEST_S2      (1)
#define GROUTE_TEST_S3      (1)
#define GROUTE_TEST_S4      (1)
#define GROUTE_TEST_S5      (1)
#define GROUTE_TEST_S6      (1)
#define GROUTE_TEST_S7      (1)
#define GROUTE_TEST_S8      (1)
#define GROUTE_TEST_S9      (1)
#define GROUTE_TEST_S10     (1)
#define GROUTE_TEST_S11     (1)
#define GROUTE_TEST_S12     (1)
#define GROUTE_TEST_S13     (1)
#define GROUTE_TEST_S14     (1)
#define GROUTE_TEST_S15     (1)
#define GROUTE_TEST_S16     (1)
#define GROUTE_TEST_S17     (1)
#define GROUTE_TEST_S18     (1)
#define GROUTE_TEST_S19     (1)
#define GROUTE_TEST_S20     (1)
#define GROUTE_TEST_S21     (1)
#define GROUTE_TEST_S22     (1)
#define GROUTE_TEST_S23     (1)
#define GROUTE_TEST_S24     (1)
#define GROUTE_TEST_S25     (1)
#define GROUTE_TEST_S26     (1)
#define GROUTE_TEST_S27     (1)
#define GROUTE_TEST_S28     (1)
#define GROUTE_TEST_S29     (1)
#define GROUTE_TEST_S30     (1)
#define GROUTE_TEST_S31     (1)
#define GROUTE_TEST_S32     (1)
#define GROUTE_TEST_S33     (1)
#define GROUTE_TEST_S34     (1)
#define GROUTE_TEST_S35     (1)
#define GROUTE_TEST_S36     (1)
#define GROUTE_TEST_S37     (1)
#define GROUTE_TEST_S38     (1)

#define GROUTE_TEST_PASS    (1)
#define GROUTE_TEST_FAIL    (0)
typedef struct {
    uint8_t addr_set;
    uint8_t addr_unset;
    uint8_t link_up;
    uint8_t link_down;
    uint8_t route_add;
    uint8_t route_del;
    uint8_t route_change;
}cb_cnt_t;
typedef struct {
    uint8_t intf;
    uint8_t route;
}reload_cnt_t;

//============= global variable =============
int g_scen = 0;
int g_case = 0;
groute_conf_cb_t    g_kernel;
groute_conf_cb_t    g_user;
cb_cnt_t            g_kernel_cnt;
cb_cnt_t            g_user_cnt;
reload_cnt_t        g_reload_cnt;
uint8_t g_kernel_find_nw;
uint8_t g_user_find_nw;
uint8_t g_kernel_addr_set;
uint8_t g_user_addr_set;
uint8_t g_new_intf_set;
uint8_t g_new_route_set;
int8_t g_acc_ret[255];
int8_t g_exp_ret[255];

int kernel_address_set(char *name, groute_addr_t addr)
{   
    g_kernel_cnt.addr_set++;
    if(!g_user_cnt.addr_set) return GROUTE_CB_FAILED;
    if(g_scen==13 && g_case==2) {
        return GROUTE_CB_FAILED;
    }
    if(g_scen==34 && g_case==1) {
        if(g_kernel_cnt.route_change) {
            return GROUTE_CB_FAILED;
        }
    }
    g_kernel_addr_set = 1;
    return GROUTE_CB_SUCCESS;
}
int kernel_address_unset(char *name, groute_addr_t addr)
{   
    g_kernel_cnt.addr_unset++;
    if(g_scen==36 && g_case==1) {
        if(!g_kernel_cnt.route_change) {
            return GROUTE_CB_FAILED;
        }
    }
    g_kernel_addr_set = 0;
    return GROUTE_CB_SUCCESS;
}
int kernel_link_set(char *name, uint8_t set)
{   
    if(set) {
        g_kernel_cnt.link_up++;
    } else {
        g_kernel_cnt.link_down++;
    }
    if(g_scen==27 || g_scen==30) {
        if(g_kernel_cnt.route_change == 0) {
            return GROUTE_CB_FAILED;
        }
    }
    return GROUTE_CB_SUCCESS;
}
int kernel_route_add(groute_addr_t dst, uint32_t metric, uint8_t nb_gw, groute_callback_gw_desc_t *gw_desc)
{   
    g_kernel_cnt.route_add++;
    g_kernel_find_nw = gw_desc->find_network;
    if(!g_user_cnt.route_add) return GROUTE_CB_FAILED;
    if(g_scen==7 && g_case==2) {
        return GROUTE_CB_FAILED;
    }

    return GROUTE_CB_SUCCESS;
}
int kernel_route_del(groute_addr_t dst, uint32_t metric)
{   
    g_kernel_cnt.route_del++;
    return GROUTE_CB_SUCCESS;
}
int kernel_route_change(groute_change_e reason, groute_addr_t dst, uint32_t metric, uint8_t nb_gw, groute_callback_gw_desc_t *gw_desc)
{   
    g_kernel_cnt.route_change++;
    return GROUTE_CB_SUCCESS;
}
int user_address_set(char *name, groute_addr_t addr)
{  
    g_user_cnt.addr_set++;
    if(g_scen==13 && g_case==1) {
        return GROUTE_CB_FAILED;
    }
    if(g_scen==34 && g_case==1) {
        if(g_kernel_cnt.route_change) {
            return GROUTE_CB_FAILED;
        }
    }
    g_user_addr_set = 1;
    return GROUTE_CB_SUCCESS;
}
int user_address_unset(char *name, groute_addr_t addr)
{   
    g_user_cnt.addr_unset++;
    g_user_addr_set = 0;
    if(g_scen==36 && g_case==1) {
        if(!g_kernel_cnt.route_change) {
            return GROUTE_CB_FAILED;
        }
    }
    return GROUTE_CB_SUCCESS;
}
int user_link_set(char *name, uint8_t set)
{   
    if(set) {
        g_user_cnt.link_up++;
    } else {
        g_user_cnt.link_down++;
    }
    if(g_scen==27 && g_scen==30) {
        if(g_user_cnt.route_change == 0) {
            return GROUTE_CB_FAILED;
        }
    }
    return GROUTE_CB_SUCCESS;
}
int user_route_add(groute_addr_t dst, uint32_t metric, uint8_t nb_gw, groute_callback_gw_desc_t *gw_desc)
{   
    g_user_cnt.route_add++;
    g_user_find_nw = gw_desc->find_network;
    if(g_scen==7 && g_case==1) {
        return GROUTE_CB_FAILED;
    }
    return GROUTE_CB_SUCCESS;
}
int user_route_del(groute_addr_t dst, uint32_t metric)
{   
    g_user_cnt.route_del++;
    return GROUTE_CB_SUCCESS;
}
int user_route_change(groute_change_e reason, groute_addr_t dst, uint32_t metric, uint8_t nb_gw, groute_callback_gw_desc_t *gw_desc)
{   
    g_user_cnt.route_change++;
    return GROUTE_CB_SUCCESS;
}
void new_intf(char *name, uint8_t admin_link, uint8_t nb_addr, groute_addr_t *addr)
{
    g_reload_cnt.intf++;
    if((strcmp("br0.0", name) == 0)) {
        if(addr->af_family == AF_INET && addr->prefix_len == 24){
            g_new_intf_set = 1;
        }
    } else {
        g_new_intf_set = 0;
    }
    return;
}
void new_route(groute_addr_t dst, uint32_t metric, uint8_t nb_gw, groute_callback_gw_desc_t *gw_desc)
{
    g_reload_cnt.route++;
    if(g_new_intf_set == 0) {
        return;
    }
    if(metric==0 && nb_gw==0) {
        if(dst.af_family == AF_INET && dst.prefix_len == 24){
            g_new_route_set = 1;
        }
    } else {
        g_new_route_set = 0;
    }
    return;
}


void checker(int len) 
{
    CU_TEST(memcmp(g_exp_ret, g_acc_ret, len) == 0);
}

void init_data_construct(groute_conf_cb_t *kernel, groute_conf_cb_t *user)
{
	kernel->address_set     = kernel_address_set; 
	kernel->address_unset   = kernel_address_unset;
	kernel->link_set        = kernel_link_set;
	kernel->route_add       = kernel_route_add;
	kernel->route_del       = kernel_route_del;
	kernel->route_change    = kernel_route_change;
	user->address_set       = user_address_set; 
	user->address_unset     = user_address_unset;
	user->link_set          = user_link_set;
	user->route_add         = user_route_add;
	user->route_del         = user_route_del;
	user->route_change      = user_route_change;
}
void reload_data_construct(groute_reload_cb_t *cb)
{
    cb->new_intf = new_intf;
    cb->new_route = new_route;
}
void addr_set_data_construct(groute_addr_t *addr, uint8_t family, char *ip_addr, uint8_t prefix)
{
    addr->af_family = family; 
    addr->prefix_len = prefix; 
    inet_pton(family, ip_addr, addr->address);
}
void gw_set_data_construct(groute_gw_desc_t *gw, uint8_t family, char *gw_addr, uint8_t weight)
{
    gw->weight = weight;
    inet_pton(family, gw_addr, gw->addr);
}

void env_init()
{
    init_data_construct(&g_kernel, &g_user);
    groute_init(&g_kernel, &g_user);
    g_kernel_addr_set   = 0;
    g_user_addr_set     = 0;
    g_kernel_find_nw    = 0;
    g_user_find_nw      = 0;
    memset(&g_kernel_cnt, 0, sizeof(cb_cnt_t));
    memset(&g_user_cnt, 0, sizeof(cb_cnt_t));
    memset(&g_reload_cnt, 0, sizeof(reload_cnt_t));
}
void env_destroy()
{
    groute_destroy();
}

void test_s1_c1()
{
    g_scen = 1;
    g_case = 1;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    kernel.address_set = NULL;
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s1_c2()
{
    g_scen = 1;
    g_case = 2;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    kernel.address_unset = NULL;
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s1_c3()
{
    g_scen = 1;
    g_case = 3;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    kernel.link_set = NULL;
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s1_c4()
{
    g_scen = 1;
    g_case = 4;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    kernel.route_add = NULL;
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s1_c5()
{
    g_scen = 1;
    g_case = 5;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    kernel.route_del = NULL;
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s1_c6()
{
    g_scen = 1;
    g_case = 6;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    kernel.route_change = NULL;
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s1_c7()
{
    g_scen = 1;
    g_case = 7;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    user.address_set = NULL;
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s1_c8()
{
    g_scen = 1;
    g_case = 8;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    user.address_unset = NULL;
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s1_c9()
{
    g_scen = 1;
    g_case = 9;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    user.link_set = NULL;
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s1_c10()
{
    g_scen = 1;
    g_case = 10;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    user.route_add = NULL;
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s1_c11()
{
    g_scen = 1;
    g_case = 11;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    user.route_del = NULL;
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s1_c12()
{
    g_scen = 1;
    g_case = 12;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    user.route_change = NULL;
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s2_c1()
{
    g_scen = 2;
    g_case = 1;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s2_c2()
{
    g_scen = 2;
    g_case = 2;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s2_c3()
{
    g_scen = 2;
    g_case = 3;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s2_c4()
{
    g_scen = 2;
    g_case = 4;
    g_exp_ret[0] = GROUTE_INIT_FAILED;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s2_c5()
{
    g_scen = 2;
    g_case = 5;
    g_exp_ret[0] = GROUTE_SUCCESS;

    groute_conf_cb_t kernel, user;
    
    init_data_construct(&kernel, &user);
    g_acc_ret[0] = groute_init(&kernel, &user);

    checker(1);
}
void test_s3_c1()
{
    g_scen = 3;
    g_case = 1;
    g_exp_ret[0] = GROUTE_ARG_FAILED;

    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    
    env_init();
    addr_set_data_construct(&dst, 0, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, 0, "10.10.10.255", 0);
    g_acc_ret[0] = groute_route_add(dst, 0, 1, &gw_desc);
    env_destroy();

    checker(1);
}
void test_s3_c2()
{
    g_scen = 3;
    g_case = 2;
    g_exp_ret[0] = GROUTE_ARG_FAILED;

    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    
    env_init();
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 48); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    g_acc_ret[0] = groute_route_add(dst, 0, 1, &gw_desc);
    env_destroy();

    checker(1);
}
void test_s3_c3()
{
    g_scen = 3;
    g_case = 3;
    g_exp_ret[0] = GROUTE_ARG_FAILED;

    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    
    env_init();
    addr_set_data_construct(&dst, AF_INET6, "2000::1", 130); 
    gw_set_data_construct(&gw_desc, AF_INET6, "2000::ff", 0);
    g_acc_ret[0] = groute_route_add(dst, 0, 1, &gw_desc);
    env_destroy();

    checker(1);
}
void test_s4_c1()
{
    g_scen = 4;
    g_case = 1;
    g_exp_ret[0] = GROUTE_ROUTE_DUP;

    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    
    env_init();
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);
    g_acc_ret[0] = groute_route_add(dst, 0, 1, &gw_desc);
    env_destroy();

    checker(1);
}
void test_s5_c1()
{
    g_scen = 5;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = GROUTE_SUCCESS;
    g_exp_ret[2] = GROUTE_SUCCESS;
    g_exp_ret[3] = GROUTE_NO_RESOURCE;

    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    
    env_init();
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    g_acc_ret[0] = groute_route_add(dst, 0, 1, &gw_desc);
    addr_set_data_construct(&dst, AF_INET, "10.10.11.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.11.255", 0);
    g_acc_ret[1] = groute_route_add(dst, 0, 1, &gw_desc);
    addr_set_data_construct(&dst, AF_INET, "10.10.12.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.12.255", 0);
    g_acc_ret[2] = groute_route_add(dst, 0, 1, &gw_desc);
    addr_set_data_construct(&dst, AF_INET, "10.10.13.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.13.255", 0);
    g_acc_ret[3] = groute_route_add(dst, 0, 1, &gw_desc);
    env_destroy();

    checker(4);
}
void test_s6_c1()
{
    g_scen = 6;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 0;
    g_exp_ret[2] = 0;

    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    
    env_init();
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    g_acc_ret[0] = groute_route_add(dst, 0, 1, &gw_desc);
    g_acc_ret[1] = g_kernel_find_nw; 
    g_acc_ret[2] = g_user_find_nw;
    env_destroy();

    checker(3);
}
void test_s6_c2()
{
    g_scen = 6;
    g_case = 2;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 1;
    g_exp_ret[2] = 1;

    groute_addr_t dst, addr;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_intf_addr_set(name, addr);
    groute_intf_link_set(name, 1);
    g_acc_ret[0] = groute_route_add(dst, 0, 1, &gw_desc);
    g_acc_ret[1] = g_kernel_find_nw; 
    g_acc_ret[2] = g_user_find_nw;
    env_destroy();

    checker(3);
}
void test_s7_c1()
{
    g_scen = 7;
    g_case = 1;
    g_exp_ret[0] = GROUTE_FAILED;
    g_exp_ret[1] = 1;
    g_exp_ret[2] = 0;

    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    
    env_init();
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    g_acc_ret[0] = groute_route_add(dst, 0, 1, &gw_desc);
    g_acc_ret[1] = g_user_cnt.route_add; 
    g_acc_ret[2] = g_kernel_cnt.route_add;
    env_destroy();

    checker(3);
}
void test_s7_c2()
{
    g_scen = 7;
    g_case = 2;
    g_exp_ret[0] = GROUTE_FAILED;
    g_exp_ret[1] = 1;
    g_exp_ret[2] = 1;
    g_exp_ret[3] = 1;

    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    
    env_init();
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    g_acc_ret[0] = groute_route_add(dst, 0, 1, &gw_desc);
    g_acc_ret[1] = g_user_cnt.route_add; 
    g_acc_ret[2] = g_user_cnt.route_del;
    g_acc_ret[3] = g_kernel_cnt.route_add;
    env_destroy();

    checker(4);
}
void test_s8_c1()
{
    g_scen = 8;
    g_case = 1;
    g_exp_ret[0] = GROUTE_ARG_FAILED;

    groute_addr_t dst;
    
    env_init();
    addr_set_data_construct(&dst, 0, "10.10.10.0", 24); 
    g_acc_ret[0] = groute_route_del(dst, 0);
    env_destroy();

    checker(1);
}
void test_s8_c2()
{
    g_scen = 8;
    g_case = 2;
    g_exp_ret[0] = GROUTE_ARG_FAILED;

    groute_addr_t dst;
    
    env_init();
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 48); 
    g_acc_ret[0] = groute_route_del(dst, 0);
    env_destroy();

    checker(1);
}
void test_s8_c3()
{
    g_scen = 8;
    g_case = 3;
    g_exp_ret[0] = GROUTE_ARG_FAILED;

    groute_addr_t dst;
    
    env_init();
    addr_set_data_construct(&dst, AF_INET6, "2000::1", 130); 
    g_acc_ret[0] = groute_route_del(dst, 0);
    env_destroy();

    checker(1);
}
void test_s9_c1()
{
    g_scen = 9;
    g_case = 1;
    g_exp_ret[0] = GROUTE_ROUTE_NOT_FOUND;

    groute_addr_t dst;
    
    env_init();
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24);
    g_acc_ret[0] = groute_route_del(dst, 100);
    env_destroy();

    checker(1);
}
void test_s10_c1()
{
    g_scen = 10;
    g_case = 1;
    g_exp_ret[0] = GROUTE_ARG_FAILED;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, 0, "10.10.10.1", 24); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    env_destroy();

    checker(1);
}
void test_s10_c2()
{
    g_scen = 10;
    g_case = 2;
    g_exp_ret[0] = GROUTE_ARG_FAILED;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 48); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    env_destroy();

    checker(1);
}
void test_s10_c3()
{
    g_scen = 10;
    g_case = 3;
    g_exp_ret[0] = GROUTE_ARG_FAILED;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET6, "2000::1", 130); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    env_destroy();

    checker(1);
}
void test_s11_c1()
{
    g_scen = 11;
    g_case = 1;
    g_exp_ret[0] = GROUTE_ADDR_DUP;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.1");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);
    addr_set_data_construct(&addr, AF_INET, "10.10.10.2", 24); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    env_destroy();

    checker(1);
}
void test_s11_c2()
{
    g_scen = 11;
    g_case = 2;
    g_exp_ret[0] = GROUTE_ADDR_DUP;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.1");
    addr_set_data_construct(&addr, AF_INET6, "2001::1", 64); 
    groute_intf_addr_set(name, addr);
    addr_set_data_construct(&addr, AF_INET6, "2001::2", 64); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    env_destroy();

    checker(1);
}
void test_s12_c1()
{
    g_scen = 12;
    g_case = 1;
    g_exp_ret[0] = GROUTE_ADDR_DUP;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);
    sprintf(name, "br0.1");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    env_destroy();

    checker(1);
}
void test_s12_c2()
{
    g_scen = 12;
    g_case = 2;
    g_exp_ret[0] = GROUTE_ADDR_DUP;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.2", 24); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    env_destroy();

    checker(1);
}
void test_s12_c3()
{
    g_scen = 12;
    g_case = 3;
    g_exp_ret[0] = GROUTE_ADDR_DUP;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET6, "2001::1", 64); 
    groute_intf_addr_set(name, addr);
    sprintf(name, "br0.1");
    addr_set_data_construct(&addr, AF_INET6, "2001::1", 64); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    env_destroy();

    checker(1);
}
void test_s12_c4()
{
    g_scen = 12;
    g_case = 4;
    g_exp_ret[0] = GROUTE_ADDR_DUP;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET6, "2001::1", 64); 
    groute_intf_addr_set(name, addr);
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET6, "2001::2", 64); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    env_destroy();

    checker(1);
}
void test_s13_c1()
{
    g_scen = 13;
    g_case = 1;
    g_exp_ret[0] = GROUTE_FAILED;
    g_exp_ret[1] = 0;
    g_exp_ret[2] = 0;
    g_exp_ret[3] = 1;
    g_exp_ret[4] = 0;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    g_acc_ret[1] = g_kernel_addr_set;
    g_acc_ret[2] = g_user_addr_set;
    g_acc_ret[3] = g_user_cnt.addr_set;
    g_acc_ret[4] = g_kernel_cnt.addr_set;
    env_destroy();

    checker(5);
}
void test_s13_c2()
{
    g_scen = 13;
    g_case = 2;
    g_exp_ret[0] = GROUTE_FAILED;
    g_exp_ret[1] = 0;
    g_exp_ret[2] = 0;
    g_exp_ret[3] = 1;
    g_exp_ret[4] = 1;
    g_exp_ret[5] = 1;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    g_acc_ret[1] = g_kernel_addr_set;
    g_acc_ret[2] = g_user_addr_set;
    g_acc_ret[3] = g_user_cnt.addr_set;
    g_acc_ret[4] = g_user_cnt.addr_unset;;
    g_acc_ret[5] = g_kernel_cnt.addr_set;
    env_destroy();

    checker(6);
}
void test_s14_c1()
{
    g_scen = 14;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = GROUTE_SUCCESS;
    g_exp_ret[2] = GROUTE_SUCCESS;
    g_exp_ret[3] = GROUTE_NO_RESOURCE;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    sprintf(name, "br0.1");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.2", 24); 
    g_acc_ret[1] = groute_intf_addr_set(name, addr);
    sprintf(name, "br0.2");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.3", 24); 
    g_acc_ret[2] = groute_intf_addr_set(name, addr);
    sprintf(name, "br0.3");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.4", 24); 
    g_acc_ret[3] = groute_intf_addr_set(name, addr);
    env_destroy();

    checker(4);
}
void test_s15_c1()
{
    g_scen = 15;
    g_case = 1;
    g_exp_ret[0] = GROUTE_ARG_FAILED;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);
    g_acc_ret[0] = groute_intf_addr_unset(name, 0);
    env_destroy();

    checker(1);
}
void test_s16_c1()
{
    g_scen = 16;
    g_case = 1;
    g_exp_ret[0] = GROUTE_ADDR_NOT_FOUND;

    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    g_acc_ret[0] = groute_intf_addr_unset(name, AF_INET);
    env_destroy();

    checker(1);
}
void test_s16_c2()
{
    g_scen = 16;
    g_case = 2;
    g_exp_ret[0] = GROUTE_ADDR_NOT_FOUND;

    char name[GROUTE_INTF_NAME_SIZE];

    env_init();
    sprintf(name, "br0.0");
    groute_intf_link_set(name, 0);
    g_acc_ret[0] = groute_intf_addr_unset(name, AF_INET);
    env_destroy();

    checker(1);
}
void test_s16_c3()
{
    g_scen = 16;
    g_case = 3;
    g_exp_ret[0] = GROUTE_ADDR_NOT_FOUND;

    char name[GROUTE_INTF_NAME_SIZE];

    env_init();
    sprintf(name, "br0.0");
    groute_intf_link_set(name, 0);
    g_acc_ret[0] = groute_intf_addr_unset(name, AF_INET6);
    env_destroy();

    checker(1);
}
void test_s17_c1()
{
    g_scen = 17;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = GROUTE_SUCCESS;

    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    groute_intf_link_set(name, 1);
    g_acc_ret[0] = groute_intf_link_set(name, 1);
    groute_intf_link_set(name, 0);
    g_acc_ret[1] = groute_intf_link_set(name, 0);
    env_destroy();

    checker(2);
}
void test_s18_c1()
{
    g_scen = 18;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = GROUTE_SUCCESS;
    g_exp_ret[2] = GROUTE_SUCCESS;
    g_exp_ret[3] = GROUTE_NO_RESOURCE;

    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    g_acc_ret[0] = groute_intf_link_set(name, 1);
    sprintf(name, "br0.1");
    g_acc_ret[1] = groute_intf_link_set(name, 1);
    sprintf(name, "br0.2");
    g_acc_ret[2] = groute_intf_link_set(name, 1);
    sprintf(name, "br0.3");
    g_acc_ret[3] = groute_intf_link_set(name, 1);
    env_destroy();

    checker(4);
}
void test_s19_c1()
{
    g_scen = 19;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    groute_reload_cb_t cb;
    
    env_init();
    reload_data_construct(&cb);
    cb.new_intf = NULL;
    g_acc_ret[0] = groute_reload(cb);
    env_destroy();

    checker(1);
}
void test_s19_c2()
{
    g_scen = 19;
    g_case = 2;
    g_exp_ret[0] = GROUTE_SUCCESS;

    groute_reload_cb_t cb;
    
    env_init();
    reload_data_construct(&cb);
    cb.new_route = NULL;
    g_acc_ret[0] = groute_reload(cb);
    env_destroy();

    checker(1);
}
void test_s20_c1()
{
    g_scen = 20;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 2;
    g_exp_ret[2] = 2;

    groute_reload_cb_t cb;
    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);

    sprintf(name, "br0.1");
    addr_set_data_construct(&addr, AF_INET6, "2001::1", 64); 
    groute_intf_addr_set(name, addr);

    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    addr_set_data_construct(&dst, AF_INET, "10.10.11.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.11.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    reload_data_construct(&cb);
    g_acc_ret[0] = groute_reload(cb);
    g_acc_ret[1] = g_reload_cnt.intf;
    g_acc_ret[2] = g_reload_cnt.route;
    env_destroy();

    checker(3);
}
void test_s21_c1()
{
    g_scen = 21;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 0;
    g_exp_ret[2] = 0;

    groute_reload_cb_t cb;
    
    env_init();
    reload_data_construct(&cb);
    g_acc_ret[0] = groute_reload(cb);
    g_acc_ret[1] = g_reload_cnt.intf;
    g_acc_ret[2] = g_reload_cnt.route;
    env_destroy();

    checker(3);
}
void test_s21_c2()
{
    g_scen = 21;
    g_case = 2;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 0;
    g_exp_ret[2] = 0;

    groute_reload_cb_t cb;
    
    env_init();
    reload_data_construct(&cb);
    g_acc_ret[0] = groute_reload(cb);
    g_acc_ret[1] = g_reload_cnt.intf;
    g_acc_ret[2] = g_reload_cnt.route;
    env_destroy();

    checker(3);
}
void test_s21_c3()
{
    g_scen = 21;
    g_case = 3;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 0;
    g_exp_ret[2] = 0;

    groute_reload_cb_t cb;
    
    env_init();
    reload_data_construct(&cb);
    g_acc_ret[0] = groute_reload(cb);
    g_acc_ret[1] = g_reload_cnt.intf;
    g_acc_ret[2] = g_reload_cnt.route;
    env_destroy();

    checker(3);
}
void test_s21_c4()
{
    g_scen = 21;
    g_case = 4;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 0;
    g_exp_ret[2] = 0;

    groute_reload_cb_t cb;
    
    env_init();
    reload_data_construct(&cb);
    g_acc_ret[0] = groute_reload(cb);
    g_acc_ret[1] = g_reload_cnt.intf;
    g_acc_ret[2] = g_reload_cnt.route;
    env_destroy();

    checker(3);
}
void test_s22_c1()
{
    g_scen = 22;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = GROUTE_SUCCESS;
    g_exp_ret[2] = GROUTE_SUCCESS;
    g_exp_ret[3] = GROUTE_SUCCESS;
    g_exp_ret[4] = GROUTE_SUCCESS;
    g_exp_ret[5] = 0;

    char name[GROUTE_INTF_NAME_SIZE];
    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    FILE *fp;

    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24);
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    addr_set_data_construct(&addr, AF_INET6, "2001::1", 64);
    g_acc_ret[1] = groute_intf_addr_set(name, addr);

    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    g_acc_ret[2] = groute_route_add(dst, 0, 1, &gw_desc);
    addr_set_data_construct(&dst, AF_INET6, "2001::1234:0", 112); 
    gw_set_data_construct(&gw_desc, AF_INET6, "2001::1234:1234", 0);
    g_acc_ret[3] = groute_route_add(dst, 0, 1, &gw_desc);

    fp = fopen("test.txt", "w+");
    g_acc_ret[4] = groute_dump(fp);
    fclose(fp);
    fp = fopen("test.txt", "r");
    g_acc_ret[5] = feof(fp);
    fclose(fp);
    env_destroy();

    checker(6);
}
void test_s23_c1()
{
    g_scen = 23;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;

    env_init();
    g_acc_ret[0] = groute_dump(stdout);
    env_destroy();

    checker(1);
}
void test_s23_c2()
{
    g_scen = 23;
    g_case = 2;
    g_exp_ret[0] = GROUTE_SUCCESS;

    env_init();
    g_acc_ret[0] = groute_dump(stdout);
    env_destroy();

    checker(1);
}
void test_s23_c3()
{
    g_scen = 23;
    g_case = 3;
    g_exp_ret[0] = GROUTE_SUCCESS;

    env_init();
    g_acc_ret[0] = groute_dump(stdout);
    env_destroy();

    checker(1);
}
void test_s23_c4()
{
    g_scen = 23;
    g_case = 4;
    g_exp_ret[0] = GROUTE_SUCCESS;

    env_init();
    g_acc_ret[0] = groute_dump(stdout);
    env_destroy();

    checker(1);
}
void test_s24_c1()
{
    g_scen = 24;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;

    env_init();
    g_acc_ret[0] = groute_dump(NULL);
    env_destroy();

    checker(1);
}
void test_s25_c1()
{
    g_scen = 25;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;

    pid_t pid;
    char name[GROUTE_INTF_NAME_SIZE];
    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    groute_reload_cb_t cb;

    env_init();
    reload_data_construct(&cb);
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24);
    groute_intf_addr_set(name, addr);
    addr_set_data_construct(&addr, AF_INET6, "2001::1", 64);
    groute_intf_addr_set(name, addr);

    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);
    addr_set_data_construct(&dst, AF_INET6, "2001::1234:0", 112); 
    gw_set_data_construct(&gw_desc, AF_INET6, "2001::1234:1234", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    sprintf(name, "br0.1");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.2", 24);
    groute_intf_addr_set(name, addr);
    addr_set_data_construct(&addr, AF_INET6, "2001::2", 64);
    groute_intf_addr_set(name, addr);

    pid = fork();
    if (pid != 0) {
        sprintf(name, "br0.1");
        g_acc_ret[0] = groute_intf_link_set(name, 0);
    } else {
        g_acc_ret[0] = groute_reload(cb);

    }
    checker(0);

    if (pid == 0) {
        pid = getpid();
        kill(pid, SIGKILL);
    }
    env_destroy();
}
void test_s26_c1()
{
    g_scen = 26;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;

    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    groute_intf_link_set(name, 1);
    g_acc_ret[0] = groute_intf_link_set(name, 0);
    env_destroy();

    checker(1);
}
void test_s26_c2()
{
    g_scen = 26;
    g_case = 2;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 1;
    g_exp_ret[2] = 1;
    g_exp_ret[3] = 1;
    g_exp_ret[4] = 1;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);

    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    g_acc_ret[0] = groute_intf_link_set(name, 0);
    g_acc_ret[1] = g_user_cnt.route_change;
    g_acc_ret[2] = g_user_cnt.link_down;
    g_acc_ret[3] = g_kernel_cnt.route_change;
    g_acc_ret[4] = g_kernel_cnt.link_down;
    env_destroy();

    checker(5);
}
void test_s27_c1()
{
    g_scen = 27;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);

    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    groute_intf_link_set(name, 1);
    g_acc_ret[0] = groute_intf_link_set(name, 0);
    env_destroy();

    checker(1);
}
void test_s28_c1()
{
    g_scen = 28;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 3;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);
    addr_set_data_construct(&addr, AF_INET6, "2000::1", 64); 
    groute_intf_addr_set(name, addr);

    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    groute_intf_link_set(name, 1);
    g_acc_ret[0] = groute_intf_link_set(name, 0);
    g_acc_ret[1] = g_kernel_cnt.addr_set;
    env_destroy();

    checker(2);
}
void test_s29_c1()
{
    g_scen = 29;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 2;
    g_exp_ret[2] = 0;
    g_exp_ret[3] = 1;
    g_exp_ret[4] = 0;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);
    groute_intf_link_set(name, 0);

    g_acc_ret[0] = groute_intf_link_set(name, 1);
    g_acc_ret[1] = g_kernel_cnt.link_up;
    g_acc_ret[2] = g_kernel_cnt.route_change;
    g_acc_ret[3] = g_user_cnt.link_up;
    g_acc_ret[4] = g_user_cnt.route_change;
    env_destroy();

    checker(5);
}
void test_s29_c2()
{
    g_scen = 29;
    g_case = 2;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 2;
    g_exp_ret[2] = 1;
    g_exp_ret[3] = 1;
    g_exp_ret[4] = 1;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);
    groute_intf_link_set(name, 0);

    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    g_acc_ret[0] = groute_intf_link_set(name, 1);
    g_acc_ret[1] = g_kernel_cnt.link_up;
    g_acc_ret[2] = g_kernel_cnt.route_change;
    g_acc_ret[3] = g_user_cnt.link_up;
    g_acc_ret[4] = g_user_cnt.route_change;
    env_destroy();

    checker(5);
}
void test_s29_c3()
{
    g_scen = 29;
    g_case = 3;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 2;
    g_exp_ret[2] = 0;
    g_exp_ret[3] = 1;
    g_exp_ret[4] = 0;

    groute_addr_t addr;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET6, "2001::1", 64); 
    groute_intf_addr_set(name, addr);
    groute_intf_link_set(name, 0);

    g_acc_ret[0] = groute_intf_link_set(name, 1);
    g_acc_ret[1] = g_kernel_cnt.link_up;
    g_acc_ret[2] = g_kernel_cnt.route_change;
    g_acc_ret[3] = g_user_cnt.link_up;
    g_acc_ret[4] = g_user_cnt.route_change;
    env_destroy();

    checker(5);
}
void test_s29_c4()
{
    g_scen = 29;
    g_case = 4;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 2;
    g_exp_ret[2] = 1;
    g_exp_ret[3] = 1;
    g_exp_ret[4] = 1;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET6, "2001::1", 64); 
    groute_intf_addr_set(name, addr);
    groute_intf_link_set(name, 0);

    addr_set_data_construct(&dst, AF_INET6, "2001::0", 64); 
    gw_set_data_construct(&gw_desc, AF_INET6, "2001::1234", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    g_acc_ret[0] = groute_intf_link_set(name, 1);
    g_acc_ret[1] = g_kernel_cnt.link_up;
    g_acc_ret[2] = g_kernel_cnt.route_change;
    g_acc_ret[3] = g_user_cnt.link_up;
    g_acc_ret[4] = g_user_cnt.route_change;
    env_destroy();

    checker(5);
}
void test_s30_c1()
{
    g_scen = 30;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);

    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    g_acc_ret[0] = groute_intf_link_set(name, 1);
    env_destroy();

    checker(1);
}
void test_s31_c1()
{
    g_scen = 31;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 1;
    g_exp_ret[2] = 1;

    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    
    env_init();
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);
    g_acc_ret[0] = groute_route_del(dst, 0);
    g_acc_ret[1] = g_user_cnt.route_del; 
    g_acc_ret[2] = g_kernel_cnt.route_del;
    env_destroy();

    checker(3);
}
void test_s32_c1()
{
    g_scen = 32;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 1;
    g_exp_ret[2] = 2;

    groute_addr_t dst;
    groute_gw_desc_t gw_desc[2];
    
    env_init();
    addr_set_data_construct(&dst, AF_INET6, "2000::1", 64); 
    gw_set_data_construct(&gw_desc[0], AF_INET6, "2000::100", 0);
    gw_set_data_construct(&gw_desc[1], AF_INET6, "2000::200", 0);
    groute_route_add(dst, 0, 2, gw_desc);
    g_acc_ret[0] = groute_route_del(dst, 0);
    g_acc_ret[1] = g_user_cnt.route_del; 
    g_acc_ret[2] = g_kernel_cnt.route_del;
    env_destroy();

    checker(3);
}
void test_s33_c1()
{
    g_scen = 33;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 1;
    g_exp_ret[2] = 1;
    g_exp_ret[3] = 1;
    g_exp_ret[4] = 1;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    //insert a route
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    addr_set_data_construct(&dst, AF_INET, "10.10.11.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.11.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    g_acc_ret[1] = g_kernel_addr_set;
    g_acc_ret[2] = g_user_addr_set;
    g_acc_ret[3] = g_kernel_cnt.route_change;
    g_acc_ret[4] = g_user_cnt.route_change;
    env_destroy();

    checker(5);
}
void test_s33_c2()
{
    g_scen = 33;
    g_case = 2;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 1;
    g_exp_ret[2] = 1;
    g_exp_ret[3] = 1;
    g_exp_ret[4] = 1;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    //insert a route
    addr_set_data_construct(&dst, AF_INET6, "2001::0", 64); 
    gw_set_data_construct(&gw_desc, AF_INET6, "2001::255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    addr_set_data_construct(&dst, AF_INET6, "2002::0", 64); 
    gw_set_data_construct(&gw_desc, AF_INET6, "2002::255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET6, "2001::1234", 64); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    g_acc_ret[1] = g_kernel_addr_set;
    g_acc_ret[2] = g_user_addr_set;
    g_acc_ret[3] = g_kernel_cnt.route_change;
    g_acc_ret[4] = g_user_cnt.route_change;
    env_destroy();

    checker(5);
}
void test_s34_c1()
{
    g_scen = 34;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 1;
    g_exp_ret[2] = 1;
    g_exp_ret[3] = 1;
    g_exp_ret[4] = 1;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    //insert a route
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    g_acc_ret[0] = groute_intf_addr_set(name, addr);
    g_acc_ret[1] = g_kernel_addr_set;
    g_acc_ret[2] = g_user_addr_set;
    g_acc_ret[3] = g_kernel_cnt.route_change;
    g_acc_ret[4] = g_user_cnt.route_change;
    env_destroy();

    checker(5);
}
void test_s35_c1()
{
    g_scen = 35;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 0;
    g_exp_ret[2] = 0;
    g_exp_ret[3] = 1;
    g_exp_ret[4] = 1;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);
    
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    g_acc_ret[0] = groute_intf_addr_unset(name, AF_INET);
    g_acc_ret[1] = g_kernel_addr_set;
    g_acc_ret[2] = g_user_addr_set;
    g_acc_ret[3] = g_kernel_cnt.route_change;
    g_acc_ret[4] = g_user_cnt.route_change;
    env_destroy();

    checker(5);
}
void test_s35_c2()
{
    g_scen = 35;
    g_case = 2;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 0;
    g_exp_ret[2] = 0;
    g_exp_ret[3] = 2;
    g_exp_ret[4] = 2;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);
    
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    sprintf(name, "br0.1");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.2", 24); 
    groute_intf_addr_set(name, addr);
    
    sprintf(name, "br0.0");
    g_acc_ret[0] = groute_intf_addr_unset(name, AF_INET);
    g_acc_ret[1] = g_kernel_addr_set;
    g_acc_ret[2] = g_user_addr_set;
    g_acc_ret[3] = g_kernel_cnt.route_change;
    g_acc_ret[4] = g_user_cnt.route_change;
    env_destroy();

    checker(5);
}
void test_s35_c3()
{
    g_scen = 35;
    g_case = 3;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 0;
    g_exp_ret[2] = 0;
    g_exp_ret[3] = 1;
    g_exp_ret[4] = 1;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET6, "2001::1", 64); 
    groute_intf_addr_set(name, addr);
    
    addr_set_data_construct(&dst, AF_INET6, "2001::0", 64); 
    gw_set_data_construct(&gw_desc, AF_INET6, "2001::1234", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    g_acc_ret[0] = groute_intf_addr_unset(name, AF_INET6);
    g_acc_ret[1] = g_kernel_addr_set;
    g_acc_ret[2] = g_user_addr_set;
    g_acc_ret[3] = g_kernel_cnt.route_change;
    g_acc_ret[4] = g_user_cnt.route_change;
    env_destroy();

    checker(5);
}
void test_s35_c4()
{
    g_scen = 35;
    g_case = 4;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 0;
    g_exp_ret[2] = 0;
    g_exp_ret[3] = 2;
    g_exp_ret[4] = 2;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET6, "2001::1", 64); 
    groute_intf_addr_set(name, addr);
    
    addr_set_data_construct(&dst, AF_INET6, "2001::0", 64); 
    gw_set_data_construct(&gw_desc, AF_INET6, "2001::1234", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    sprintf(name, "br0.1");
    addr_set_data_construct(&addr, AF_INET6, "2001::2", 64); 
    groute_intf_addr_set(name, addr);
    
    sprintf(name, "br0.0");
    g_acc_ret[0] = groute_intf_addr_unset(name, AF_INET6);
    g_acc_ret[1] = g_kernel_addr_set;
    g_acc_ret[2] = g_user_addr_set;
    g_acc_ret[3] = g_kernel_cnt.route_change;
    g_acc_ret[4] = g_user_cnt.route_change;
    env_destroy();

    checker(5);
}
void test_s36_c1()
{
    g_scen = 36;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = 0;
    g_exp_ret[2] = 0;
    g_exp_ret[3] = 1;
    g_exp_ret[4] = 1;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    env_init();
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    groute_intf_addr_set(name, addr);
    
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    groute_route_add(dst, 0, 1, &gw_desc);

    g_acc_ret[0] = groute_intf_addr_unset(name, AF_INET);
    g_acc_ret[1] = g_kernel_addr_set;
    g_acc_ret[2] = g_user_addr_set;
    g_acc_ret[3] = g_kernel_cnt.route_change;
    g_acc_ret[4] = g_user_cnt.route_change;
    env_destroy();

    checker(5);
}

void test_s37_c1()
{
    g_scen = 37;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;

    g_acc_ret[0] = groute_init(NULL, NULL);
    g_acc_ret[0] = groute_init(NULL, NULL);
    sleep(1);
    env_destroy();

    checker(1);
}
void test_s37_c2()
{
    g_scen = 37;
    g_case = 2;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = GROUTE_SUCCESS;
    g_exp_ret[2] = GROUTE_SUCCESS;
    g_exp_ret[3] = GROUTE_SUCCESS;
    g_exp_ret[4] = GROUTE_SUCCESS;
    g_exp_ret[5] = GROUTE_SUCCESS;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    g_acc_ret[0] = groute_init(NULL, NULL);
    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    g_acc_ret[1] = groute_intf_addr_set(name, addr);
    
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    g_acc_ret[2] = groute_route_add(dst, 0, 1, &gw_desc);

    g_acc_ret[3] = groute_intf_link_set(name, 0);
    g_acc_ret[4] = groute_intf_addr_unset(name, AF_INET);
    g_acc_ret[5] = groute_route_del(dst, 0);
    env_destroy();

    checker(6);
}

void test_s38_c1()
{
    g_scen = 38;
    g_case = 1;
    g_exp_ret[0] = GROUTE_SUCCESS;
    g_exp_ret[1] = GROUTE_SUCCESS;
    g_exp_ret[2] = GROUTE_SUCCESS;
    g_exp_ret[3] = GROUTE_SUCCESS;
    g_exp_ret[4] = GROUTE_SUCCESS;
    g_exp_ret[5] = GROUTE_SUCCESS;
    g_exp_ret[6] = GROUTE_SUCCESS;
    g_exp_ret[7] = GROUTE_SUCCESS;
    g_exp_ret[8] = GROUTE_SUCCESS;
    g_exp_ret[9] = GROUTE_SUCCESS;
    g_exp_ret[10] = GROUTE_SUCCESS;

    groute_addr_t addr;
    groute_addr_t dst;
    groute_gw_desc_t gw_desc;
    char name[GROUTE_INTF_NAME_SIZE];
    
    g_acc_ret[0] = groute_init(NULL, NULL);
    g_acc_ret[0] = groute_clear();

    sprintf(name, "br0.0");
    addr_set_data_construct(&addr, AF_INET, "10.10.10.1", 24); 
    g_acc_ret[2] = groute_intf_addr_set(name, addr);
    
    addr_set_data_construct(&dst, AF_INET, "10.10.10.0", 24); 
    gw_set_data_construct(&gw_desc, AF_INET, "10.10.10.255", 0);
    g_acc_ret[3] = groute_route_add(dst, 0, 1, &gw_desc);
    g_acc_ret[4] = groute_init(NULL, NULL);
    g_acc_ret[5] = groute_intf_link_set(name, 0);
    g_acc_ret[6] = groute_init(NULL, NULL);
    g_acc_ret[7] = groute_intf_addr_unset(name, AF_INET);
    g_acc_ret[8] = groute_init(NULL, NULL);
    g_acc_ret[9] = groute_route_del(dst, 0);
    g_acc_ret[10] = groute_init(NULL, NULL);
    env_destroy();

    checker(10);
}

int main() 
{
    system("clear");
    printf("Starting GRoute Testing...\n");

	CU_pSuite pSuite = NULL;
	CU_pTest  pTest  = NULL;
	if (CUE_SUCCESS != CU_initialize_registry()){//Init the registry
		return CU_get_error();
	}
	
#ifdef GROUTE_TEST_S1
	pSuite = CU_add_suite("GROUTE.S1: [v1.0-1]callback function null", NULL, NULL);//Add the suite
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    
    pTest = CU_add_test(pSuite, "case 1-1: kernel address_set is null", test_s1_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 1-2: kernel address_unset is null", test_s1_c2);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 1-3: kernel link_set is null", test_s1_c3);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 1-4: kernel route_add is null", test_s1_c4);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 1-5: kernel route_del is null", test_s1_c5);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 1-6: kernel route_change is null", test_s1_c6);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 1-7: user address_set is null", test_s1_c7);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 1-8: user address_unset is null", test_s1_c8);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 1-9: user link_set is null", test_s1_c9);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 1-10: user route_add is null", test_s1_c10);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 1-11: user route_del is null", test_s1_c11);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 1-12: user route_change is null", test_s1_c12);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S2 
    pSuite = CU_add_suite("GROUTE.S2: [v1.0-2]resource allocation", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 2-1: allocate resource failed (shmget failed)", test_s2_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 2-1: allocate resource failed (shmget failed)", test_s2_c2);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
    pTest = CU_add_test(pSuite, "case 2-1: allocate resource failed (shmat failed)", test_s2_c3);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
    pTest = CU_add_test(pSuite, "case 2-1: allocate resource failed (shmat failed)", test_s2_c4);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
    pTest = CU_add_test(pSuite, "case 2-5: init success", test_s2_c5);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S3 
    pSuite = CU_add_suite("GROUTE.S3: [v1.0-21]af_family test", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 3-1: AF_FAMILY != (AF_INET || AF_INET6)", test_s3_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 3-2: AF_FAMILY = AF_INET and prefix_len > 32", test_s3_c2);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 3-3: AF_FAMILY = AF_INET6 and prefix_len >128", test_s3_c3);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S4
    pSuite = CU_add_suite("GROUTE.S4: [v1.0-22]duplicate route", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 4-1: route with same dst, prefix and metric already existing", test_s4_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S5
    pSuite = CU_add_suite("GROUTE.S5: [v1.0-23]internal storage full", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 5-1: internal data storage full", test_s5_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S6
    pSuite = CU_add_suite("GROUTE.S6: [v1.0-24]route_add cb execution", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 6-1: find_network is false", test_s6_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 6-1: find_network is true", test_s6_c2);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S7
    pSuite = CU_add_suite("GROUTE.S7: [v1.0-25]route_add  2 cb functions sync test", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 7-1: user callback return GROUTE_CB_FAILED", test_s7_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 7-2: user cb success, but kernel cb failed", test_s7_c2);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S8
    pSuite = CU_add_suite("GROUTE.S8: [v1.0-26]af_family test", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 8-1: AF_FAMILY != (AF_INET || AF_INET6)", test_s8_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 8-2: AF_FAMILY = AF_INET and prefix_len > 32", test_s8_c2);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 8-3: AF_FAMILY = AF_INET6 and prefix_len > 128", test_s8_c3);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S9
    pSuite = CU_add_suite("GROUTE.S9: [v1.0-27]route not found", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 9-1: delete an unset route", test_s9_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S10
    pSuite = CU_add_suite("GROUTE.S10: [v1.0-3]af_family test", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 10-1: AF_FAMILY != (AF_INET || AF_INET6)", test_s10_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 10-2: AF_FAMILY = AF_INET and prefix_len > 32", test_s10_c2);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 10-3: AF_FAMILY = AF_INET6 and prefix_len >128", test_s10_c3);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S11 
    pSuite = CU_add_suite("GROUTE.S11: [v1.0-4]set addr of same network twice", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 11-1: set br0.1 ipv4 addr twice", test_s11_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}

    pTest = CU_add_test(pSuite, "case 11-2: set br0.1 ipv6 addr twice", test_s11_c2);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
#endif

#ifdef GROUTE_TEST_S12
    pSuite = CU_add_suite("GROUTE.S12: [v1.0-5]set addr same addr exists", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 12-1: set br0.0 and br0.1 with the same ipv4 addr", test_s12_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 12-2: set br0.0 ipv4 address twice", test_s12_c2);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
    pTest = CU_add_test(pSuite, "case 12-3: set br0.0 and br0.1 with the same ipv6 addr", test_s12_c3);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
    pTest = CU_add_test(pSuite, "case 12-4: set br0.0 ipv6 address twice", test_s12_c4);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
#endif

#ifdef GROUTE_TEST_S13
    pSuite = CU_add_suite("GROUTE.S13: [v1.0-6]set addr two callback function sync test", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 13-1: user callback return GROUTE_CB_FAILED", test_s13_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 13-2: user cb success, but kernel cb failed", test_s13_c2);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S14
    pSuite = CU_add_suite("GROUTE.S14: [v1.0-9]internal data storage test", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 14-1: internal data storage full", test_s14_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S15
    pSuite = CU_add_suite("GROUTE.S15: [v1.0-10]af_family test", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 15-1: AF_FAMILY != (AF_INET || AF_INET6)", test_s15_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S16
    pSuite = CU_add_suite("GROUTE.S16: [v1.0-11]unset addr not found", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 16-1: unset an un-configured interface address", test_s16_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 16-2: unset an un-configured ipv4 address", test_s16_c2);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
    pTest = CU_add_test(pSuite, "case 16-3: unset an un-configured ipv6 address", test_s16_c3);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
#endif

#ifdef GROUTE_TEST_S17
    pSuite = CU_add_suite("GROUTE.S17: [v1.0-14]same request as previous", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 17-1: config is unchanged and return success", test_s17_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S18
    pSuite = CU_add_suite("GROUTE.S18: [v1.0-20]link up internal storage full", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 18-1: internal data storage full", test_s18_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S19
    pSuite = CU_add_suite("GROUTE.S19: [v1.0-32]cb ptr check", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 19-1: callback function new_intf is NULL", test_s19_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 19-2: callback function new_route is NULL", test_s19_c2);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S20
    pSuite = CU_add_suite("GROUTE.S20: [v1.0-30]reload config to other process", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 20-1: ", test_s20_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S21
    pSuite = CU_add_suite("GROUTE.S21: [v1.0-31]get config from IPC failed", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 21-1: shmget failed", test_s21_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 21-2: shmat failed", test_s21_c2);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 21-3: shmget failed", test_s21_c3);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
    pTest = CU_add_test(pSuite, "case 21-4: shmat failed", test_s21_c4);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
#endif

#ifdef GROUTE_TEST_S22
    pSuite = CU_add_suite("GROUTE.S22: [v1.0-33]dump config", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 22-1: dump config", test_s22_c1);
    if (pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
#endif

#ifdef GROUTE_TEST_S23
    pSuite = CU_add_suite("GROUTE.S23: [v1.0-34]get config from IPC failed", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 23-1: shmget failed", test_s23_c1);
    if (pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
    pTest = CU_add_test(pSuite, "case 23-2: shmget failed", test_s23_c2);
    if (pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
    pTest = CU_add_test(pSuite, "case 23-3: shmat failed", test_s23_c3);
    if (pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
    pTest = CU_add_test(pSuite, "case 23-4: shmat failed", test_s23_c4);
    if (pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
#endif

#ifdef GROUTE_TEST_S24
    pSuite = CU_add_suite("GROUTE.S24: [v1.0-35]file ptr check", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 24-1: NULL file pointer to dump", test_s24_c1);
    if (pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
#endif

#ifdef GROUTE_TEST_S25
    pSuite = CU_add_suite("GROUTE.S25: [v1.0-36]IPC executing protection", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 25-1: Config and reload by 2 processes", test_s25_c1);
    if (pTest == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }
#endif


#ifdef GROUTE_TEST_S26
    pSuite = CU_add_suite("GROUTE.S26: [v1.0-15]link down route change test", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 26-1: link down with no route referencing to this interface", test_s26_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 26-2: link down with routes referencing to this interface", test_s26_c2);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S27
    pSuite = CU_add_suite("GROUTE.S27: [v1.0-16]link down cb execute sequence", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 27-1: execute route_change cb then link_set", test_s27_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S28
    pSuite = CU_add_suite("GROUTE.S28: [v1.0-17]link down recover IPv6 addr", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 28-1: link-downed interface contains IPv6 addr", test_s28_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S29
    pSuite = CU_add_suite("GROUTE.S29: [v1.0-18]link up route change test", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 29-1: link up with no ipv4 route referencing this interface", test_s29_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 29-2: link up with ipv4 route referencing this interface", test_s29_c2);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}

    pTest = CU_add_test(pSuite, "case 29-1: link up with no ipv6 route referencing this interface", test_s29_c3);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
    pTest = CU_add_test(pSuite, "case 29-2: link up with ipv6 route referencing this interface", test_s29_c4);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
#endif

#ifdef GROUTE_TEST_S30
    pSuite = CU_add_suite("GROUTE.S30: [v1.0-19]link up cb execute sequence", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 30-1: link up with no route referencing this interface", test_s30_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S31
    pSuite = CU_add_suite("GROUTE.S31: [v1.0-28]route_del cb execute sequence", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 31-1: call user cb first then kernel cb", test_s31_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S32
    pSuite = CU_add_suite("GROUTE.S32: [v1.0-29]route_del dealing with IPv6 ECMP route", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 32-1: call kernel cb according to gateway number", test_s32_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S33
    pSuite = CU_add_suite("GROUTE.S33: [v1.0-7]output network test", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 33-1: execute route_change when ipv4 route can find output network again", test_s33_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 33-2: execute route_change when ipv6 route can find output network again", test_s33_c2);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
#endif

#ifdef GROUTE_TEST_S34
    pSuite = CU_add_suite("GROUTE.S34: [v1.0-8]cb execute sequence", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 34-1: execute address set before route change", test_s34_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S35
    pSuite = CU_add_suite("GROUTE.S35: [v1.0-12]output network test", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 35-1: ipv4 route cannot find output network", test_s35_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 35-2: ipv4 route can find another output network", test_s35_c2);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 35-3: ipv6 route cannot find output network", test_s35_c3);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
    pTest = CU_add_test(pSuite, "case 35-4: ipv6 route can find another output network", test_s35_c4);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
#endif

#ifdef GROUTE_TEST_S36
    pSuite = CU_add_suite("GROUTE.S36: [v1.0-13]cb execute sequence", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}
    pTest = CU_add_test(pSuite, "case 36-1: execute route_change before address_unset", test_s36_c1);
	if(pTest == NULL){
		CU_cleanup_registry();
		return CU_get_error();
	}
#endif

#ifdef GROUTE_TEST_S37
    pSuite = CU_add_suite("GROUTE.S37: [v1.0-3] Initialzation function without parameters as input", NULL, NULL);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    pTest = CU_add_test(pSuite, "case 37-1: Initialzation function without any callback function", test_s37_c1);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }

    pTest = CU_add_test(pSuite, "case 37-2: Groute works when not prepareing any callback function", test_s37_c2);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
#endif

#ifdef GROUTE_TEST_S38
    pSuite = CU_add_suite("GROUTE.S38: Configuration between multiple Initialzation.", NULL, NULL);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    pTest = CU_add_test(pSuite, "case 38-1: Insert groute_init between all ", test_s38_c1);
    if(pTest == NULL){
        CU_cleanup_registry();
        return CU_get_error();
    }
#endif

#ifndef UNIT_TEST_FAST
#ifdef TEST_BASIC
	CU_basic_set_mode(CU_BRM_VERBOSE);
	//CU_basic_set_mode(CU_BRM_NORMAL);
	//CU_basic_set_mode(CU_BRM_SILENT);
	CU_basic_run_tests();
	CU_cleanup_registry();
#else
	CU_console_run_tests();
#endif
#else
	CU_set_output_filename("groute_test");
	//CU_list_tests_to_file();
	CU_automated_run_tests();
#endif

    return CU_get_number_of_failures();
}
