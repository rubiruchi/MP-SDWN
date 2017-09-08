/*
 * hostapd / Initialization and configuration
 * Copyright (c) 2002-2013, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2017, liyaming <liyaming@gmail.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef HOSTAPD_H
#define HOSTAPD_H

#include <libubox/avl.h>
#include <libubus.h>

#include "../utils/common.h"
#include "sta_info.h"
#include "ieee802_1x_defs.h"
#include "../drivers/driver.h"
#include "ap_config.h"
#include "config_file.h"
#include <json-c/json.h>

#define HOSTAPD_RATE_BASIC 0x00000001

enum hostapd_chan_status {
	HOSTAPD_CHAN_VALID = 0, /* channel is ready */
	HOSTAPD_CHAN_INVALID = 1, /* no usable channel found */
	HOSTAPD_CHAN_ACS = 2, /* ACS work being performed */
};

enum Hostapd_iface_state{
		HAPD_IFACE_UNINITIALIZED,
		HAPD_IFACE_DISABLED,
		HAPD_IFACE_COUNTRY_UPDATE,
		HAPD_IFACE_ACS,
		HAPD_IFACE_HT_SCAN,
		HAPD_IFACE_DFS,
		HAPD_IFACE_ENABLED
	};
	
struct hostapd_ubus_bss {
	struct ubus_object obj;
	struct avl_tree banned;
};

struct hostapd_ubus_iface {
	struct ubus_object obj;
};

struct hostapd_rate_data {
	int rate; /* rate in 100 kbps */
	int flags; /* HOSTAPD_RATE_ flags */
};

struct hapd_interfaces {
	int (*reload_config)(struct hostapd_iface *iface);
	struct hostapd_config * (*config_read_cb)(const char *config_fname);
	int (*ctrl_iface_init)(struct hostapd_data *hapd);
	void (*ctrl_iface_deinit)(struct hostapd_data *hapd);
	int (*for_each_interface)(struct hapd_interfaces *interfaces,
				int (*cb)(struct hostapd_iface *iface,void *ctx), 
				void *ctx);
	int (*driver_init)(struct hostapd_iface *iface);

	size_t count;
	int global_ctrl_sock;
	char *global_iface_path;
	char *global_iface_name;
#ifndef CONFIG_NATIVE_WINDOWS
	gid_t ctrl_iface_group;
#endif /* CONFIG_NATIVE_WINDOWS */
	struct hostapd_iface **iface;

	size_t terminate_on_error;
};

/**
 * struct hostapd_iface - hostapd per-interface data structure
 */
struct hostapd_iface {
	struct hapd_interfaces *interfaces;
	void *owner;
	char *config_fname;
	struct hostapd_config *conf;
	char phy[16]; /* Name of the PHY (radio) */

	struct hostapd_ubus_iface ubus;

	enum Hostapd_iface_state state;

	size_t num_bss;
	struct hostapd_data **bss;//hostapd_data ������һ��BSS

	unsigned int wait_channel_update:1;
	unsigned int cac_started:1;
	
    /*
	 * When set, indicates that the driver will handle the AP
	 * teardown: delete global keys, station keys, and stations.
	 */
	unsigned int driver_ap_teardown:1;

	int num_ap; /* number of entries in ap_list */
	struct ap_info *ap_list; /* AP info list head */
	struct ap_info *ap_hash[STA_HASH_SIZE];

	unsigned int drv_flags;

	/*
	 * A bitmap of supported protocols for probe response offload. See
	 * struct wpa_driver_capa in driver.h
	 */
	unsigned int probe_resp_offloads;

	/* extended capabilities supported by the driver */
	const u8 *extended_capa, *extended_capa_mask;
	unsigned int extended_capa_len;

	unsigned int drv_max_acl_mac_addrs;

	struct hostapd_hw_modes *hw_features;
	int num_hw_features;
	struct hostapd_hw_modes *current_mode;
	/* Rates that are currently used (i.e., filtered copy of
	 * current_mode->channels */
	int num_rates;
	struct hostapd_rate_data *current_rates;
	int *basic_rates;
	int freq;

	u16 hw_flags;

	/* Number of associated Non-ERP stations (i.e., stations using 802.11b
	 * in 802.11g BSS) */
	int num_sta_non_erp;

	/* Number of associated stations that do not support Short Slot Time */
	int num_sta_no_short_slot_time;

	/* Number of associated stations that do not support Short Preamble */
	int num_sta_no_short_preamble;

	int olbc; /* Overlapping Legacy BSS Condition */

	/* Number of HT associated stations that do not support greenfield */
	int num_sta_ht_no_gf;

	/* Number of associated non-HT stations */
	int num_sta_no_ht;

	/* Number of HT associated stations 20 MHz */
	int num_sta_ht_20mhz;

	/* Number of HT40 intolerant stations */
	int num_sta_ht40_intolerant;

	/* Overlapping BSS information */
	int olbc_ht;

	u16 ht_op_mode;

	/* surveying helpers */

	/* number of channels surveyed */
	unsigned int chans_surveyed;

	/* lowest observed noise floor in dBm */
	s8 lowest_nf;

	/* channel switch parameters */
	struct hostapd_freq_params cs_freq_params;
	u8 cs_count;
	int cs_block_tx;
	unsigned int cs_c_off_beacon;
	unsigned int cs_c_off_proberesp;
	int csa_in_progress;

	unsigned int dfs_cac_ms;
	struct os_reltime dfs_cac_start;

	/* Latched with the actual secondary channel information and will be
	 * used while juggling between HT20 and HT40 modes. */
	int secondary_ch;

#ifdef CONFIG_ACS
	unsigned int acs_num_completed_scans;
#endif /* CONFIG_ACS */

	void (*scan_cb)(struct hostapd_iface *iface);
	int num_ht40_scan_tries;
};

u32 hostapd_sta_flags_to_drv(u32 flags);

int hostapd_get_frame_socket_fd(struct hostapd_data *hapd);

int hostapd_recv_mgmt_frame(struct hostapd_data *hapd);

void  hostapd_get_ht_capab(struct hostapd_data *hapd,
				  struct ieee80211_ht_capabilities *ht_cap,
				  struct ieee80211_ht_capabilities *neg_ht_cap);
	
void hostapd_get_vht_capab(struct hostapd_data *hapd,
				   struct ieee80211_vht_capabilities *vht_cap,
				   struct ieee80211_vht_capabilities *neg_vht_cap);				  
int hostapd_drv_sta_remove(struct hostapd_data *hapd,
						 const u8 *addr);
	
int hostapd_set_sta_flags(struct hostapd_data *hapd, struct sta_info *sta);

struct sta_info * ap_sta_add(struct hostapd_data *hapd, const u8 *addr);
	
void handle_auth(struct hostapd_data *hapd,const u8 *addr,int auth_alg);
	
int hostapd_sta_add(struct hostapd_data *hapd,
		    const u8 *addr, u16 aid, u16 capability,
		    const u8 *supp_rates, size_t supp_rates_len,
		    u16 listen_interval,
		    const struct ieee80211_ht_capabilities *ht_capab,
		    const struct ieee80211_vht_capabilities *vht_capab,
		    u32 flags, u8 qosinfo, u8 vht_opmode, int session);
	
u16 check_assoc_ies(struct hostapd_data *hapd, struct sta_info *sta,
			   const u8 *ies, size_t ies_len, int reassoc);
	
u8* hostapd_handle_assoc(struct hostapd_data *hapd,
			 const u8 *packet, int len, u8 *vbssid,
			 int reassoc,int *frame_len);	

void hostapd_handle_assoc_cb(struct hostapd_data *hapd, const u8 *addr);

int hostapd_driver_init(struct hostapd_iface *iface);		
	
int hostapd_drv_send_mlme(struct hostapd_data *hapd,
			  const u8 *msg, size_t len, int noack);

/**
 * struct hostapd_data - hostapd per-BSS data structure
 */
struct hostapd_data{
	struct hostapd_iface *iface;
	struct hostapd_config *iconf;
	struct hostapd_bss_config *conf;
	struct hostapd_ubus_bss ubus;
	int interface_added; /* virtual interface added for this BSS */
	unsigned int started:1;

	u8 own_addr[ETH_ALEN];

	int num_sta; /* number of entries in sta_list */
	struct sta_info *sta_list; /* STA info list head */
#define STA_HASH_SIZE 256
#define STA_HASH(sta) (sta[5])
	struct sta_info *sta_hash[STA_HASH_SIZE];

	/*
	 * Bitfield for indicating which AIDs are allocated. Only AID values
	 * 1-2007 are used and as such, the bit at index 0 corresponds to AID
	 * 1.
	 */
#define AID_WORDS ((2008 + 31) / 32)
	u32 sta_aid[AID_WORDS];

	const struct wpa_driver_ops *driver;
	void *drv_priv;

	void (*new_assoc_sta_cb)(struct hostapd_data *hapd,
				 struct sta_info *sta, int reassoc);

	void *msg_ctx; /* ctx for wpa_msg() calls */
	void *msg_ctx_parent; /* parent interface ctx for wpa_msg() calls */

	struct radius_client_data *radius;
	u32 acct_session_id_hi, acct_session_id_lo;
	struct radius_das_data *radius_das;

	struct iapp_data *iapp;

	struct hostapd_cached_radius_acl *acl_cache;
	struct hostapd_acl_query_data *acl_queries;

	struct wpa_authenticator *wpa_auth;
	struct eapol_authenticator *eapol_auth;

	struct rsn_preauth_interface *preauth_iface;
	struct os_reltime michael_mic_failure;
	int michael_mic_failures;
	int tkip_countermeasures;

	int ctrl_sock;
	struct wpa_ctrl_dst *ctrl_dst;

	void *ssl_ctx;
	void *eap_sim_db_priv;
	struct radius_server_data *radius_srv;

	int parameter_set_count;

	/* Time Advertisement */
	u8 time_update_counter;
	struct wpabuf *time_adv;

#ifdef CONFIG_FULL_DYNAMIC_VLAN
	struct full_dynamic_vlan *full_dynamic_vlan;
#endif /* CONFIG_FULL_DYNAMIC_VLAN */

	struct l2_packet_data *l2;
	struct wps_context *wps;

	int beacon_set_done;
	struct wpabuf *wps_beacon_ie;
	struct wpabuf *wps_probe_resp_ie;
#ifdef CONFIG_WPS
	unsigned int ap_pin_failures;
	unsigned int ap_pin_failures_consecutive;
	struct upnp_wps_device_sm *wps_upnp;
	unsigned int ap_pin_lockout_time;

	struct wps_stat wps_stats;
#endif /* CONFIG_WPS */

	struct hostapd_probereq_cb *probereq_cb;
	size_t num_probereq_cb;

	void (*public_action_cb)(void *ctx, const u8 *buf, size_t len,
				 int freq);
	void *public_action_cb_ctx;
	void (*public_action_cb2)(void *ctx, const u8 *buf, size_t len,
				  int freq);
	void *public_action_cb2_ctx;

	int (*vendor_action_cb)(void *ctx, const u8 *buf, size_t len,
				int freq);
	void *vendor_action_cb_ctx;

	//void (*wps_reg_success_cb)(void *ctx, const u8 *mac_addr,
	//			   const u8 *uuid_e);
	void *wps_reg_success_cb_ctx;

	//void (*wps_event_cb)(void *ctx, enum wps_event event,
	//		     union wps_event_data *data);
	void *wps_event_cb_ctx;

	void (*sta_authorized_cb)(void *ctx, const u8 *mac_addr,
				  int authorized, const u8 *p2p_dev_addr);
	void *sta_authorized_cb_ctx;

	void (*setup_complete_cb)(void *ctx);
	void *setup_complete_cb_ctx;

	void (*new_psk_cb)(void *ctx, const u8 *mac_addr,
			   const u8 *p2p_dev_addr, const u8 *psk,
			   size_t psk_len);
	void *new_psk_cb_ctx;

#ifdef CONFIG_P2P
	struct p2p_data *p2p;
	struct p2p_group *p2p_group;
	struct wpabuf *p2p_beacon_ie;
	struct wpabuf *p2p_probe_resp_ie;

	/* Number of non-P2P association stations */
	int num_sta_no_p2p;

	/* Periodic NoA (used only when no non-P2P clients in the group) */
	int noa_enabled;
	int noa_start;
	int noa_duration;
#endif /* CONFIG_P2P */
#ifdef CONFIG_INTERWORKING
	size_t gas_frag_limit;
#endif /* CONFIG_INTERWORKING */

#ifdef CONFIG_SQLITE
	struct hostapd_eap_user tmp_eap_user;
#endif /* CONFIG_SQLITE */

#ifdef CONFIG_SAE
	/** Key used for generating SAE anti-clogging tokens */
	u8 sae_token_key[8];
	struct os_reltime last_sae_token_key_update;
#endif /* CONFIG_SAE */

#ifdef CONFIG_TESTING_OPTIONS
	int ext_mgmt_frame_handling;
#endif /* CONFIG_TESTING_OPTIONS */
};

struct hostapd_data *
hostapd_alloc_bss_data(struct hostapd_iface *hapd_iface,
		       struct hostapd_config *conf,
		       struct hostapd_bss_config *bss);

u16 copy_supp_rates(struct hostapd_data *hapd, struct sta_info *sta,
			   struct ieee802_11_elems *elems);

u16 copy_sta_ht_capab(struct hostapd_data *hapd, struct sta_info *sta,
		      const u8 *ht_capab, size_t ht_capab_len);

int hostapd_get_aid(struct hostapd_data *hapd, struct sta_info *sta);

int hostapd_read_all_sta_data(struct hostapd_data *hapd, 
            struct hostap_sta_list *sta_list);

struct hostapd_iface * hostapd_init(struct hapd_interfaces *interfaces,
				    const char *config_file);

struct hostapd_iface *
hostapd_interface_init(struct hapd_interfaces *interfaces,
		       const char *config_fname, int debug);

void hostapd_interface_init_bss(struct hapd_interfaces *interfaces);

int hostapd_send_csa_action_frame(struct hostapd_data *hapd, 
            const u8 *addr, const u8 *bssid,
            const u8 block_tx, const u8 new_channel, const u8 cs_count);

void hostapd_inf_init(struct hapd_interfaces *interfaces);

int hostapd_setup_interface(struct hostapd_iface *iface);

int hostapd_get_mgmt_socket_fd(struct hostapd_data *hapd);


#endif
