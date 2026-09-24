#include <stdint.h>
#include <stddef.h>
#include "identity_engine.h"
#include "../include/rig_http_client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

static const char CURP_CHARSET[] __attribute__((unused)) =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ\xc3\x91";

static int curp_char_value(char c) {

    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'N') return c - 'A' + 10;
    if (c == '\xd1' || c == '\xc3') return 36;
    if (c >= 'O' && c <= 'Z') return c - 'O' + 25;
    return -1;
}

id_err_t nr_id_validate_curp_checksum(const char *curp) {
    if (!curp) return ID_ERR_FORMAT;

    char upper[20];
    size_t len = strlen(curp);
    if (len != 18) return ID_ERR_FORMAT;

    for (size_t i = 0; i < 18; i++) upper[i] = (char)toupper((unsigned char)curp[i]);
    upper[18] = '\0';

    int sum = 0;
    for (int i = 0; i < 17; i++) {
        int val = curp_char_value(upper[i]);
        if (val < 0) return ID_ERR_FORMAT;
        sum += val * (18 - i);
    }

    int expected_check = (10 - (sum % 10)) % 10;
    int provided_check = upper[17] - '0';

    return (expected_check == provided_check) ? ID_OK : ID_ERR_FORMAT;
}

id_err_t nr_id_validate_ine(const char  *curp,
                              const char  *cic,
                              const char  *renapo_url,
                              const char  *renapo_token,
                              nr_ine_t    *out) {
    if (!curp || !cic) return ID_ERR_FORMAT;
    memset(out, 0, sizeof(*out));

    id_err_t chk = nr_id_validate_curp_checksum(curp);
    if (chk != ID_OK) return chk;

    size_t cic_len = strlen(cic);
    if (cic_len < 9 || cic_len > 11) return ID_ERR_FORMAT;
    for (size_t i = 0; i < cic_len; i++) {
        if (!isdigit((unsigned char)cic[i])) return ID_ERR_FORMAT;
    }

    snprintf(out->curp, sizeof(out->curp), "%s", curp);
    snprintf(out->cic,  sizeof(out->cic),  "%s", cic);
    out->valid_checksum = 1;

    if (!renapo_url || !*renapo_url) return ID_OK;

    char url[512];
    snprintf(url, sizeof(url), "%s/%s", renapo_url, curp);

    nr_http_response_t resp;
    int rc = nr_http_post_json(url, renapo_token, "{}", &resp);
    if (rc != 0) {

        fprintf(stderr, "[identity] RENAPO unreachable (%s) — accepting structural check\n",
                resp.error);
        return ID_OK;
    }

    if (strstr(resp.body, "\"error\"") || resp.body[0] == '\0') return ID_ERR_NOT_FOUND;

    return ID_OK;
}

static int mrz_char_value(char c) {
    if (c == '<') return 0;
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

static int mrz_check_digit(const char *s, size_t len) {
    static const int W[3] = {7, 3, 1};
    int sum = 0;
    for (size_t i = 0; i < len; i++) {
        int v = mrz_char_value(s[i]);
        if (v < 0) return -1;
        sum += v * W[i % 3];
    }
    return sum % 10;
}

id_err_t nr_id_validate_passport_mrz(const char    *line1,
                                       const char    *line2,
                                       nr_passport_t *out) {
    if (!line1 || !line2 || !out) return ID_ERR_FORMAT;
    if (strlen(line1) != 44 || strlen(line2) != 44) return ID_ERR_FORMAT;

    memset(out, 0, sizeof(*out));
    out->check_digits_ok = 1;

    char doc_num[10]; memcpy(doc_num, line2, 9); doc_num[9] = '\0';
    int cd_doc = mrz_check_digit(doc_num, 9);
    if (cd_doc < 0 || (line2[9] - '0') != cd_doc) out->check_digits_ok = 0;
    snprintf(out->document_number, sizeof(out->document_number), "%s", doc_num);

    char dob[7]; memcpy(dob, line2 + 13, 6); dob[6] = '\0';
    int cd_dob = mrz_check_digit(dob, 6);
    if (cd_dob < 0 || (line2[19] - '0') != cd_dob) out->check_digits_ok = 0;
    snprintf(out->date_of_birth, sizeof(out->date_of_birth), "%s", dob);

    char exp[7]; memcpy(exp, line2 + 21, 6); exp[6] = '\0';
    int cd_exp = mrz_check_digit(exp, 6);
    if (cd_exp < 0 || (line2[27] - '0') != cd_exp) out->check_digits_ok = 0;
    snprintf(out->expiry_date, sizeof(out->expiry_date), "%s", exp);

    char nat[4]; memcpy(nat, line2 + 10, 3); nat[3] = '\0';
    snprintf(out->nationality, sizeof(out->nationality), "%s", nat);

    char names[40]; memcpy(names, line1 + 5, 39); names[39] = '\0';
    char *sep = strstr(names, "<<");
    if (sep) {
        *sep = '\0';
        snprintf(out->surname,     sizeof(out->surname),     "%s", names);
        snprintf(out->given_names, sizeof(out->given_names), "%s", sep + 2);

        for (char *p = out->surname;     *p; p++) if (*p == '<') *p = ' ';
        for (char *p = out->given_names; *p; p++) if (*p == '<') *p = ' ';
    }

    return out->check_digits_ok ? ID_OK : ID_ERR_FORMAT;
}

static const char RFC_TABLE[] =
    "0123456789ABCDEFGHIJKLMN&OPQRSTUVWXYZ Ñ";

id_err_t nr_id_validate_rfc(const char *rfc) {
    if (!rfc) return ID_ERR_FORMAT;

    char upper[16];
    size_t len = strlen(rfc);
    if (len != 12 && len != 13) return ID_ERR_FORMAT;

    for (size_t i = 0; i < len; i++) upper[i] = (char)toupper((unsigned char)rfc[i]);
    upper[len] = '\0';

    size_t alpha_prefix = (len == 13) ? 4 : 3;
    for (size_t i = 0; i < alpha_prefix; i++) {
        if (!isalpha((unsigned char)upper[i])) return ID_ERR_FORMAT;
    }

    for (size_t i = alpha_prefix; i < alpha_prefix + 6; i++) {
        if (!isdigit((unsigned char)upper[i])) return ID_ERR_FORMAT;
    }

    int sum = 0;
    for (size_t i = 0; i < len - 1; i++) {
        const char *pos = strchr(RFC_TABLE, upper[i]);
        if (!pos) return ID_ERR_FORMAT;
        int val = (int)(pos - RFC_TABLE);
        sum += val * (int)(len - i);
    }
    int remainder = sum % 11;
    char expected = remainder == 0 ? '0'
                  : remainder == 1 ? 'A'
                  : (char)('0' + (11 - remainder));

    return (upper[len-1] == expected) ? ID_OK : ID_ERR_FORMAT;
}

#define SHA256_BLOCK_SZ  64
#define SHA256_DIG_SZ    32

typedef struct {
    uint32_t state[8]; uint64_t count;
    uint8_t buf[SHA256_BLOCK_SZ]; uint32_t buflen;
} _sha256_id_t;

static const uint32_t _K_id[64]={
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,
    0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
    0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,
    0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,
    0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
    0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,
    0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,
    0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
    0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};

#define _ROR(x,n) (((x)>>(n))|((x)<<(32-(n))))
static void _sha256_xform(_sha256_id_t *c,const uint8_t *d){
    uint32_t a,b,cc,e2,f,g,h,t1,t2,m[64]; int i;
    for(i=0;i<16;i++) m[i]=((uint32_t)d[i*4]<<24)|((uint32_t)d[i*4+1]<<16)|((uint32_t)d[i*4+2]<<8)|d[i*4+3];
    for(;i<64;i++) m[i]=(_ROR(m[i-2],17)^_ROR(m[i-2],19)^(m[i-2]>>10))+m[i-7]+(_ROR(m[i-15],7)^_ROR(m[i-15],18)^(m[i-15]>>3))+m[i-16];
    a=c->state[0];b=c->state[1];cc=c->state[2];e2=c->state[3];f=c->state[4];g=c->state[5];h=c->state[6];
    for(i=0;i<64;i++){t1=h+(_ROR(f,6)^_ROR(f,11)^_ROR(f,25))+((f&g)^(~f&h))+_K_id[i]+m[i];
    t2=(_ROR(a,2)^_ROR(a,13)^_ROR(a,22))+((a&b)^(a&cc)^(b&cc));h=g;g=f;f=e2;e2=e2+t1;cc=b;b=a;a=t1+t2;}
    c->state[0]+=a;c->state[1]+=b;c->state[2]+=cc;c->state[3]+=e2;c->state[4]+=f;c->state[5]+=g;c->state[6]+=h;}

static void _sha256_init(_sha256_id_t *c){c->count=0;c->buflen=0;
    c->state[0]=0x6a09e667;c->state[1]=0xbb67ae85;c->state[2]=0x3c6ef372;c->state[3]=0xa54ff53a;
    c->state[4]=0x510e527f;c->state[5]=0x9b05688c;c->state[6]=0x1f83d9ab;c->state[7]=0x5be0cd19;}

static void _sha256_upd(_sha256_id_t *c,const uint8_t *d,size_t l){
    while(l--){c->buf[c->buflen++]=*d++;if(c->buflen==SHA256_BLOCK_SZ){_sha256_xform(c,c->buf);c->count+=512;c->buflen=0;}}}

static void _sha256_fin(_sha256_id_t *c,uint8_t *o){
    uint32_t i=c->buflen;c->buf[i++]=0x80;
    if(i>56){while(i<SHA256_BLOCK_SZ)c->buf[i++]=0;_sha256_xform(c,c->buf);i=0;}
    while(i<56){ c->buf[i++]=0; } c->count+=(uint64_t)c->buflen*8;
    for(int k=7;k>=0;k--)c->buf[56+(7-k)]=(uint8_t)(c->count>>(k*8));
    _sha256_xform(c,c->buf);
    for(i=0;i<8;i++){o[i*4]=(uint8_t)(c->state[i]>>24);o[i*4+1]=(uint8_t)(c->state[i]>>16);o[i*4+2]=(uint8_t)(c->state[i]>>8);o[i*4+3]=(uint8_t)(c->state[i]);}}

static void _hmac(const uint8_t *key,size_t kl,const uint8_t *msg,size_t ml,uint8_t *out){
    uint8_t k[SHA256_BLOCK_SZ]={0},ip[SHA256_BLOCK_SZ],op[SHA256_BLOCK_SZ],inner[SHA256_DIG_SZ];
    _sha256_id_t c;
    if(kl>SHA256_BLOCK_SZ){_sha256_init(&c);_sha256_upd(&c,key,kl);_sha256_fin(&c,k);}else memcpy(k,key,kl);
    for(int i=0;i<SHA256_BLOCK_SZ;i++){ip[i]=k[i]^0x36;op[i]=k[i]^0x5c;}
    _sha256_init(&c);_sha256_upd(&c,ip,SHA256_BLOCK_SZ);_sha256_upd(&c,msg,ml);_sha256_fin(&c,inner);
    _sha256_init(&c);_sha256_upd(&c,op,SHA256_BLOCK_SZ);_sha256_upd(&c,inner,SHA256_DIG_SZ);_sha256_fin(&c,out);}

#define _hmac_fixed(key,kl,msg,ml,out) do { \
    uint8_t _k[SHA256_BLOCK_SZ]={0},_ip[SHA256_BLOCK_SZ],_op[SHA256_BLOCK_SZ],_in[SHA256_DIG_SZ]; \
    _sha256_id_t _c; \
    if((kl)>SHA256_BLOCK_SZ){_sha256_init(&_c);_sha256_upd(&_c,(const uint8_t*)(key),(kl));_sha256_fin(&_c,_k);}else memcpy(_k,(key),(kl)); \
    for(int _i=0;_i<SHA256_BLOCK_SZ;_i++){_ip[_i]=_k[_i]^0x36;_op[_i]=_k[_i]^0x5c;} \
    _sha256_init(&_c);_sha256_upd(&_c,_ip,SHA256_BLOCK_SZ);_sha256_upd(&_c,(const uint8_t*)(msg),(ml));_sha256_fin(&_c,_in); \
    _sha256_init(&_c);_sha256_upd(&_c,_op,SHA256_BLOCK_SZ);_sha256_upd(&_c,_in,SHA256_DIG_SZ);_sha256_fin(&_c,(out)); \
} while(0)

int nr_id_sign_consent(const char *user_id,
                        const char *phone_number,
                        const char *session_id,
                        long        timestamp,
                        const char *consent_secret,
                        char       *out,
                        size_t      out_len) {
    if (!user_id || !phone_number || !session_id || !consent_secret || !out) return -1;
    if (out_len < 65) return -1;

    char msg[512];
    snprintf(msg, sizeof(msg), "%s|%s|%s|%ld", user_id, phone_number, session_id, timestamp);

    uint8_t raw[SHA256_DIG_SZ];
    _hmac_fixed(consent_secret, strlen(consent_secret),
                msg, strlen(msg), raw);

    for (int i = 0; i < SHA256_DIG_SZ; i++) snprintf(out + i*2, 3, "%02x", raw[i]);
    out[64] = '\0';
    return 0;
}

int nr_id_verify_consent(const char *user_id,
                          const char *phone_number,
                          const char *session_id,
                          long        timestamp,
                          const char *consent_secret,
                          const char *signature) {
    if (!signature || strlen(signature) != 64) return 0;
    char expected[65];
    if (nr_id_sign_consent(user_id, phone_number, session_id, timestamp,
                           consent_secret, expected, sizeof(expected)) != 0) return 0;

    volatile int diff = 0;
    for (int i = 0; i < 64; i++) diff |= (unsigned char)expected[i] ^ (unsigned char)signature[i];
    return diff == 0;
}
