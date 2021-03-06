#ifndef __PEER_H__
#define __PEER_H__

#define PEER_RTP_CTX_COUNT 8
#define PEER_RTP_CTX_WRITE 4

#define PEER_RTP_SEQ_MIN_RECLAIMABLE 128

#define PEER_LOCK(x) pthread_mutex_lock(&peers[(x)].mutex)
#define PEER_UNLOCK(x) pthread_mutex_unlock(&peers[(x)].mutex)

#define PEER_THREAD_LOCK(x) pthread_mutex_lock(&((x)->mutex))
#define PEER_THREAD_UNLOCK(x) pthread_mutex_unlock(&((x)->mutex))

#define PEER_BUFFER_NODE_BUFLEN 4096

typedef struct peer_buffer_node
{
    struct peer_buffer_node* next, *tail;

    char buf[PEER_BUFFER_NODE_BUFLEN];
    unsigned int len;
    unsigned int id;
    unsigned long seq;
    unsigned long timestamp;
    unsigned long timestamp_delta;
    unsigned long timestamp_delta_initial;
    unsigned long recv_time;
    unsigned long recv_time_delta;
    u8 rtp_payload_type;
    int type;
    int rtp_idx;
    int consumed;
    int head_inited;
    int reclaimable;
} peer_buffer_node_t;

typedef struct
{
    u32 stunID32;

    struct sockaddr_in addr;
    struct sockaddr_in addr_listen;

    struct {
        char key[64];
    } crypto;

    struct {
        int bound;
        int reverse_bind;
        int bound_client;
        char ufrag_offer[64];
        char ufrag_answer[64];
        char offer_pwd[128];
        char answer_pwd[128];
        unsigned long bind_req_rtt;
        int bind_req_calc;
        char uname[64];
    } stun_ice;

    struct {
        SSL *ssl;
        char mk_label[64];
        char master_key_salt[SRTP_MASTER_KEY_KEY_LEN*2 + SRTP_MASTER_KEY_SALT_LEN * 2];
        char master_key_pad[8];
        char *master_key[2] /* client, server */; 
        char *master_salt[2] /* client, server */;
        int connected;
        int state;
        int use_membio;
    } dtls;

    struct {
        srtp_policy_t policy;
        srtp_ctx_t ctx;
        srtp_t session;
        char keybuf[64];
        unsigned long ssrc;
        int idx_write;
        int offset_subscription;
        unsigned long timestamp_subscription;
        u16 seq_counter;
        int inited;
        u32 ts_last_unprotect;
        int ffwd_done;
        unsigned long recv_time_avg;
        unsigned long ts_last;
        u8 recv_report_buflast[2048];
        u32 recv_report_buflast_len;
        u32 recv_report_seqlast;
        u32 recv_report_tslast;

        time_t pli_last;
    } srtp[PEER_RTP_CTX_COUNT];

    int subscriptionID;
    int subscribed;
    peer_buffer_node_t *subscription_ptr[PEER_RTP_CTX_COUNT];

    rtp_state_t rtp_states[PEER_RTP_CTX_COUNT];

    struct {
        //char in[2048];
        char out[2048];
        //int in_len;
        int out_len;
    } bufs;

    struct {
        char buf[4096];
        int len;
    } cleartext;

    peer_buffer_node_t in_buffers_head;
    peer_buffer_node_t rtp_buffers_head[PEER_RTP_CTX_COUNT];
    peer_buffer_node_t *rtp_buffers_tailcache[PEER_RTP_CTX_COUNT];
    unsigned int rtp_buffered_total;
    u32 rtp_timestamp_initial[PEER_RTP_CTX_COUNT];
    u16 rtp_seq_initial[PEER_RTP_CTX_COUNT];

    int sock;

    time_t time_pkt_last;
    time_t time_cleanup_last;
    time_t time_start;

    pthread_t thread;
    pthread_t thread_rtp_send;
    pthread_mutex_t mutex;

    int fwd;

    int alive, running;

    int srtp_inited;

    volatile int cleanup_in_progress;

    int id;
    char room_name[64];

    unsigned long time_last_run;
    unsigned long in_rate_ms;

    struct {
        unsigned long stat[8];
    } stats;

    char name[64];

    int recv_only;
} peer_session_t;

extern unsigned long get_time_ms();

extern void peer_buffer_node_list_init(peer_buffer_node_t* head);

void peer_init(peer_session_t* peer, int id)
{
    memset(peer, 0, sizeof(*peer));

    peer->id = id;

    int i;

    for(i = 0; i < PEER_RTP_CTX_COUNT; i++) peer_buffer_node_list_init(&peer->rtp_buffers_head[i]);
    peer_buffer_node_list_init(&peer->in_buffers_head);

    peer->time_start = time(NULL);
}

static unsigned long
peer_subscription_ts_initial(peer_session_t* peers, int id, int stream_id)
{
    return peers[id].rtp_timestamp_initial[stream_id];
}

static peer_buffer_node_t*
peer_subscription(peer_session_t* peers, int id, int stream_id, peer_buffer_node_t** pos)
{
    peer_buffer_node_t* head = &(peers[id].rtp_buffers_head[stream_id]);

    if(*pos == NULL)
    {
        *pos = head;
    }

    if(!(*pos)->next) return NULL;
    else *pos = (*pos)->next;

    return (*pos);
}

int peer_cleanup_in_progress(peer_session_t* peers, int id) {
    return peers[id].cleanup_in_progress;
}

#endif
