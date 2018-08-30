#include "eosdacrandom.hpp"
#include <eosiolib/system.h>
#include <eosiolib/stdlib.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/symbol.hpp>
#include "../oracleserver/oracleserver.hpp"

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

void eosdacrandom::setreserved(vector<name> dfs)
{
    require_auth(_self);

    if (dfs.empty()) {
        return;
    }

    seedconfig_table config(_self, _self);
    auto existing = config.find(_self);
    eosio_assert(existing != config.end(), "target size must set first");
    eosio_assert(dfs.size() <= existing->target_size, "reserved data feeders' number bigger than target size");
    eosio_assert(dfs.size() <= existing->target_size - existing->hash_count, "too much reserved data feeders");

    seed_table seeds(_self, _self);
    name n;
    n.value = _self;
    for (const auto& df : dfs) {
        dfreg_table dfregs(N(oracleserver), df);
        auto df_existing = dfregs.find(n);
        eosio_assert(df_existing != dfregs.end(), "data feeder must register first");

        auto sd = seeds.find(df);
        if (sd == seeds.end()) {
            seeds.emplace(_self, [&](auto& a){
                a.datafeeder = df;
            });

            // do we have to set config's state? like hash_count? I think we do.
            config.modify(*existing, _self, [&](auto& c){
                c.hash_count++;
            });
        }
    }
}

void eosdacrandom::sendseed(name datafeeder, string seed)
{
    require_auth(datafeeder);

    name n;
    n.value = _self;

    dfreg_table dfregs(N(oracleserver), datafeeder);
    auto df_existing = dfregs.find(n);
    eosio_assert(df_existing != dfregs.end(), "data feeder is not registered to oracleserver");

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
        // datafeeder is evil, kick him out, whole procedure start over.
        for (auto itr = seeds.cbegin(); itr != seeds.cend(); ) {
            itr = seeds.erase(itr);
        }
        config.modify(cfg, _self, [&](auto& c){
            c.hash_count = 0;
            c.seed_match = 0;
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
        // commit random to consumer.
        dispatch_request();

        vector<feedback> fbs;
        for (auto itr = seeds.begin(); itr != seeds.end(); ++itr) {
            // unfreeze datafeeder's stake balance.
            dispatch_inline(N(oracleserver), N(unfreezedf),
                                {permission_level(_self, N(active))},
                                std::make_tuple(_self, itr->datafeeder, asset(100000, S(4,OCT))));

            feedback fb{itr->datafeeder, asset(10000, S(4,OCT)), 1};
            fbs.push_back(fb);
        }

        // feedback about datafeeders.
        dispatch_inline(N(oracleserver), N(feedbackdf),
                                {permission_level(_self, N(active))},
                                std::make_tuple(_self, fbs));
    }
}

void eosdacrandom::sendhash(name datafeeder, string hash)
{
    require_auth(datafeeder);

    name n;
    n.value = _self;

    dfreg_table dfregs(N(oracleserver), datafeeder);
    auto df_existing = dfregs.find(n);
    eosio_assert(df_existing != dfregs.end(), "data feeder is not registered to oracleserver");

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

    order_table orders(N(oracleserver), consumer);
    bool found = false;
    for (const auto& o : orders) {
        if (o.orderid == orderid) {
            found = true;
        }
    }
    eosio_assert(found, "order id is not exist");

    geter_table geters(_self, _self);
    auto it = geters.find(consumer);
    uint64_t cur = current_time();
    requestinfo req {cur, orderid};
    if (it == geters.end()) {
        geters.emplace(_self, [&](auto& a){
            a.consumer = consumer;
            a.requestinfos.push_back(req);
        });
    } else {
        geters.modify(it, _self, [&](auto& a){
            bool same_order = false;
            for (auto ri = a.requestinfos.begin(); ri != a.requestinfos.end(); ++ri) {
                if (ri->orderid == orderid) {    // if orderid equals former orderid.
                    *ri = req;
                    same_order = true;
                    break;
                }
            }

            if (!same_order) {
                a.requestinfos.push_back(req);
            }
        });
    }
}

string eosdacrandom::one_seed()
{
    // use seeds to generate random number
    seedconfig_table config(_self, _self);
    auto existing = config.find(_self);
    eosio_assert(existing != config.end(), "target size must set first");
    eosio_assert(existing->hash_count == existing->target_size, "seed is not full");

    // how to generate random number?
    string  seed;
    seed_table seeds(_self, _self);
    for (auto it = seeds.cbegin(); it != seeds.cend();) {
        seed += it->seed;
        it = seeds.erase(it);
    }

    config.modify(existing, _self, [&](auto& c){
        c.hash_count = 0;
        c.seed_match = 0;
    });

    return seed;
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

string eosdacrandom::cal_sha256_str(string seed)
{
    string h;
    char d[255] = { 0 };
    snprintf(d, sizeof(d) - 1, "%s", seed.c_str());
    checksum256 cs = { 0 };
    sha256(d, strlen(d), &cs);
    for (int i = 0; i < sizeof(cs.hash); ++i) {
        char hex[3] = { 0 };
        snprintf(hex, sizeof(hex), "%02x", static_cast<unsigned char>(cs.hash[i]));
        h += hex;
    }

    return h;
}

string eosdacrandom::make_sha256_str(int index, string orderid, string seed)
{
    string h;
    char str[255] = { 0 };
    snprintf(str, sizeof(str), "%d%s%s", index, orderid.c_str(), seed.c_str());
    checksum256 cs = { 0 };
    sha256(str, strlen(str), &cs);

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
    string seed = one_seed();
    static int expiraion = 3000; // ms
    std::vector<std::vector<requestinfo>> reqs;

    geter_table geters(_self, _self);
    for (auto i = geters.cbegin(); i != geters.cend();) {
        auto tmp = i->requestinfos;
        int index = 0;
        for (auto j = tmp.begin(); j != tmp.end();) {
            if (cur - j->timestamp >= expiraion) {
                string random = make_sha256_str(index++, j->orderid, seed);
                dispatch_inline(i->consumer, N(genrandom),
                                {permission_level(_self, N(active))},
                                std::make_tuple(j->orderid, random));
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

EOSIO_ABI( eosdacrandom, (setsize) (setreserved) (sendseed) (sendhash) (regrequest) )
