#include "eosdacrandom.hpp"
#include <eosiolib/system.h>
#include <eosiolib/stdlib.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/symbol.hpp>
#include "../eosdactoken/eosdactoken.hpp"
#include "../oracleserver/oracleserver.hpp"

using namespace eosio;

eosdacrandom::eosdacrandom(account_name name)
        : contract(name)
{
}

eosdacrandom::~eosdacrandom()
{
}

void eosdacrandom::setsize(uint64_t size)
{
    require_auth(_self);

    seedconfig_table config(_self, _self);
    auto existing = config.find(_self);
    if (existing != config.end()) {
        if (existing->hash_count == existing->target_size) {
            if (existing->hash_count != existing->seed_match) {
                eosio_assert(false, "do not set size during sendseed period");
            }
        }

        config.modify(existing, _self, [&](auto& c){
            c.target_size = size;
        });
    } else {
        config.emplace(_self, [&](auto& c){
            c.owner = _self;
            c.target_size = size;
        });
    }
}

void eosdacrandom::sendseed(name datafeeder, int64_t seed)
{
    require_auth(datafeeder);

    name n;
    n.value = _self;
    bool df_validate = oracleserver(tokenContract).datafeedervalidate(datafeeder, n);
    eosio_assert(df_validate, "data feeder is not registered to oracleserver");

    seedconfig_table config(_self, _self);
    auto existing = config.find(_self);
    eosio_assert(existing != config.end(), "target size must set first");
    const auto& cfg = *existing;

    seed_table seeds(_self, _self);
    const auto& sd = seeds.find(datafeeder);
    if (cfg.hash_count < cfg.target_size) {
        if (sd != seeds.end()) {
            seeds.erase(sd);
            config.modify(cfg, _self, [&](auto& c){
                c.hash_count --;
            });
        }
        eosio_assert(false, "hash size not fulled");
    }

    string h = cal_sha256_str(seed);
    eosio_assert(sd->seed != seed, "you have already send seed");
    if (sd->hash != h) {
        for (auto itr = seeds.cbegin(); itr != seeds.cend(); ) {
            itr = seeds.erase(itr);
        }
        config.modify(cfg, _self, [&](auto& c){
            c.hash_count = 0;
        });
        return;
    }

    seeds.modify(sd, _self, [&](auto& a){
        a.seed = seed;
    });

    config.modify(cfg, _self, [&](auto& c){
        c.seed_match = c.seed_match + 1;
        print(c.seed_match);
    });

    // if all seeds match
    if (seedsmatch()) {
        dispatch_request();
    }
}

void eosdacrandom::sendhash(name datafeeder, string hash)
{
    require_auth(datafeeder);

    name n;
    n.value = _self;
    bool df_validate = oracleserver(tokenContract).datafeedervalidate(datafeeder, n);
    eosio_assert(df_validate, "data feeder is not registered to oracleserver");

    seedconfig_table config(_self, _self);
    auto existing = config.find(_self);
    eosio_assert(existing != config.end(), "target size must set first");
    const auto& cfg = *existing;

    eosio_assert(cfg.hash_count < cfg.target_size, "seeds is full");

    seed_table seeds(_self, _self);
    auto s = seeds.find(datafeeder);
    if (s == seeds.end()) {
        seeds.emplace(_self, [&](auto& a){
            a.datafeeder = datafeeder;
            a.hash = hash;
        });

        config.modify(cfg, _self, [&](auto& c){
            c.hash_count++;
        });
    } else {
        seeds.modify(s, _self, [&](auto& a){
            a.hash = hash;
        });
    }
}

void eosdacrandom::regrequest(name consumer, string orderid)
{
    eosio_assert(is_account(consumer), "Invalid account");

    // here we should query consumer and orderid from oracleserver for answer, whether the orderid is valid.
    // if true, then go ahead, otherwise it stops.

    bool order_validate = oracleserver(tokenContract).orderidvalidate(consumer, orderid);
    eosio_assert(order_validate, "order id is not exist");

    geter_table geters(_self, _self);
    auto it = geters.find(consumer);
    uint64_t cur = current_time();
    request_info req {cur, orderid};
    if (it == geters.end()) {
        geters.emplace(_self, [&](auto& a){
            a.consumer = consumer;
            a.requestinfo.push_back(req);
        });
    } else {
        geters.modify(it, _self, [&](auto& a){
            bool same_order = false;
            for (auto ri = a.requestinfo.begin(); ri != a.requestinfo.end(); ++ri) {
                if (ri->orderid == orderid) {    // if orderid equals former orderid.
                    *ri = req;
                    same_order = true;
                    break;
                }
            }

            if (!same_order) {
                a.requestinfo.push_back(req);
            }
        });
    }
}

int64_t eosdacrandom::random()
{
    // use seeds to generate random number
    seedconfig_table config(_self, _self);
    auto existing = config.find(_self);
    eosio_assert(existing != config.end(), "target size must set first");
    eosio_assert(existing->hash_count == existing->target_size, "seed is not full");

    // how to generate random number?
    int64_t  seed = 0;
    seed_table seeds(_self, _self);
    for (auto it = seeds.cbegin(); it != seeds.cend();) {
        seed += it->seed;
        it = seeds.erase(it);
    }

    config.modify(existing, _self, [&](auto& c){
        c.hash_count = 0;
        c.seed_match = 0;
    });

    checksum256 cs = cal_sha256(seed);
    return (((int64_t)cs.hash[0] << 56 ) & 0xFF00000000000000U)
        |  (((int64_t)cs.hash[1] << 48 ) & 0x00FF000000000000U)
        |  (((int64_t)cs.hash[2] << 40 ) & 0x0000FF0000000000U)
        |  (((int64_t)cs.hash[3] << 32 ) & 0x000000FF00000000U)
        |  ((cs.hash[4] << 24 )          & 0x00000000FF000000U)
        |  ((cs.hash[5] << 16 )          & 0x0000000000FF0000U)
        |  ((cs.hash[6] << 8 )           & 0x000000000000FF00U)
        |  (cs.hash[7]                   & 0x00000000000000FFU);
}

bool eosdacrandom::seedsmatch()
{
    seedconfig_table config(_self, _self);
    auto existing = config.find(_self);
    eosio_assert(existing != config.end(), "target size must set first");
    const auto& cfg = *existing;

    if (cfg.hash_count != cfg.target_size) {
        return false;
    }

    if (cfg.hash_count == cfg.seed_match) {
        return true;
    }

    return false;
}

checksum256 eosdacrandom::cal_sha256(int64_t word)
{
    checksum256 cs = { 0 };
    char d[255] = { 0 };
    snprintf(d, sizeof(d) - 1, "%lld", word);
    sha256(d, strlen(d), &cs);

    return cs;
}

string eosdacrandom::cal_sha256_str(int64_t word)
{
    string h;
    checksum256 cs = cal_sha256(word);
    for (int i = 0; i < sizeof(cs.hash); ++i) {
        char hex[3] = { 0 };
        snprintf(hex, sizeof(hex), "%02x", static_cast<unsigned char>(cs.hash[i]));
        h += hex;
    }

    return h;
}

void eosdacrandom::dispatch_request()
{
    print("all seeds matched.");
    uint64_t cur = current_time();
    int64_t num = random();
    static int expiraion = 3000; // ms
    std::vector<std::vector<request_info>> reqs;

    geter_table geters(_self, _self);
    for (auto i = geters.cbegin(); i != geters.cend();) {
        std:vector<request_info> tmp(i->requestinfo);
        for (auto j = tmp.begin(); j != tmp.end();) {
            if (cur - j->timestamp >= expiraion) {
                dispatch_inline(i->consumer, string_to_name("getrandom"),
                                {permission_level(_self, N(active))},
                                std::make_tuple(j->orderid, num));
                j = tmp.erase(j);
            } else {
                ++j;
            }
        }

        if (tmp.size()) {
            reqs.push_back(tmp);
        }

        i = geters.erase(i);
    }
}

EOSIO_ABI( eosdacrandom, (setsize) (sendseed) (sendhash) (regrequest) )
