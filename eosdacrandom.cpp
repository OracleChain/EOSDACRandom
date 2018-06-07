#include "eosdacrandom.hpp"
#include <eosiolib/system.h>
#include <eosiolib/stdlib.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/symbol.hpp>
#include "../eosdactoken/eosdactoken.hpp"

using namespace eosio;

eosdacrandom::eosdacrandom(account_name name)
        : contract(name),
          _seeds(_self, name),
          _geters(_self, name)
{
    print("eosdacrandom construct");
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
            c.target_size = size;
        });
    }
}

void eosdacrandom::sendseed(name owner, int64_t seed, string sym)
{
    eosio_assert(is_account(owner), "Invalid account");
    symbol_type symbol(string_to_symbol(4, sym.c_str()));
    eosio::asset fromBalance = eosdactoken(N(eosdactoken)).get_balance(owner, symbol.name());
    eosio_assert(fromBalance.amount > 0, "account has not enough OCT to do it");

    eosio::asset selfBalance = eosdactoken(N(eosdactoken)).get_balance(_self, symbol.name());
    eosio_assert(selfBalance.amount > 0, "contract account has not enough OCT to do it");

    require_auth(owner);

    seedconfig_table config(_self, _self);
    auto existing = config.find(_self);
    eosio_assert(existing != config.end(), "target size must set first");
    const auto& cfg = *existing;

    const auto& sd = _seeds.find(owner);
    if (cfg.hash_count < cfg.target_size) {
        print("hash count: ");
        print(cfg.hash_count);
        print(" target size: ");
        print(cfg.target_size);
        print(" hash count < target size");
        if (sd != _seeds.end()) {
            _seeds.erase(sd);
            config.modify(cfg, _self, [&](auto& c){
                c.hash_count --;
            });
        }
        eosio_assert(false, "hash size not fulled");
    }

    string h = cal_sha256_str(seed);
    eosio_assert(sd->seed != seed, "you have already send seed");
    if (sd->hash != h) {
        print("seed not match hash");
        //SEND_INLINE_ACTION( eosdacvote, vote, {_self,N(active)}, {_self, owner, selfBalance, false} );
        for (auto itr = _seeds.cbegin(); itr != _seeds.cend(); ) {
            itr = _seeds.erase(itr);
        }
        config.modify(cfg, _self, [&](auto& c){
            c.hash_count = 0;
        });
        return;
    }

    print("hash match. ");

    _seeds.modify(sd, _self, [&](auto& a){
        print("modify. ");
        a.seed = seed;
    });

    print("modify done. ");

    config.modify(cfg, _self, [&](auto& c){
        c.seed_match++;
    });

    print("seed match");

    // if all seeds match
    if (seedsmatch()) {
        print("all seeds matched.");
        uint64_t cur = current_time();
        int64_t num = random();
        static int expiraion = 3000000; // ms

        for (auto i = _geters.cbegin(); i != _geters.cend(); ++i) {
            bool dispatched = false;
            std:vector<request_info> tmp(i->requestinfo);
            for (auto j = tmp.begin(); j != tmp.end();) {
                if (cur - j->timestamp >= expiraion) {
                    dispatch_inline(i->owner, string_to_name("getrandom"),
                                    {permission_level(_self, N(active))},
                                    std::make_tuple(j->index, num));
                    j = tmp.erase(j);
                    dispatched = true;
                } else {
                    ++j;
                }
            }

            if (dispatched) {
                _geters.erase(i);

                if (tmp.size()) {
                    _geters.emplace(_self, [&](auto& a){
                        a.owner = owner;
                        a.requestinfo = tmp;
                    });
                }
            }
        }
    }

    print("sendseed");
}

void eosdacrandom::sendhash(name owner, string hash, string sym)
{
    print("###enter sendhash###");
    eosio_assert(is_account(owner), "Invalid account");

    seedconfig_table config(_self, _self);
    auto existing = config.find(_self);
    eosio_assert(existing != config.end(), "target size must set first");
    const auto& cfg = *existing;

    eosio_assert(cfg.hash_count < cfg.target_size, "seeds is full");

    symbol_type symbol(string_to_symbol(4, sym.c_str()));
    eosio::asset fromBalance = eosdactoken(N(eosdactoken)).get_balance(owner, symbol.name());
    eosio_assert(fromBalance.amount > 0, "account has not enough OCT to do it");

    require_auth(owner);

    auto s = _seeds.find(owner);
    if (s == _seeds.end()) {
        _seeds.emplace(_self, [&](auto& a){
            a.owner = owner;
            a.hash = hash;
        });

        config.modify(cfg, _self, [&](auto& c){
            c.hash_count++;
        });
    } else {
        _seeds.modify(s, _self, [&](auto& a){
            a.hash = hash;
        });
    }
}

void eosdacrandom::regrequest(name owner, uint64_t index)
{
    eosio_assert(is_account(owner), "Invalid account");

    uint64_t cur = current_time();
    auto it = _geters.find(owner);
    request_info req {index, cur};
    if (it == _geters.end()) {
        _geters.emplace(_self, [&](auto& a){
            a.owner = owner;
            a.requestinfo.push_back(req);
        });
    } else {
        _geters.modify(it, _self, [&](auto& a){
            bool same_idx = false;
            for (auto ri = a.requestinfo.begin(); ri != a.requestinfo.end(); ++ri) {
                if (ri->index == index) {    // if index equals former index.
                    *ri = req;
                    same_idx = true;
                    break;
                }
            }

            if (!same_idx) {
                a.requestinfo.push_back(req);
            }
        });
    }
    print("regrequest");
}

int64_t eosdacrandom::random()
{
    // use _seeds to generate random number
    seedconfig_table config(_self, _self);
    auto existing = config.find(_self);
    eosio_assert(existing != config.end(), "target size must set first");
    eosio_assert(existing->hash_count == existing->target_size, "seed is not full");

    // how to generate random number?
    int64_t  seed = 0;
    for (auto it = _seeds.cbegin(); it != _seeds.cend();) {
        seed += it->seed;
        it = _seeds.erase(it);
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
        |  ((cs.hash[4] << 24 ) & 0x00000000FF000000U)
        |  ((cs.hash[5] << 16 ) & 0x0000000000FF0000U)
        |  ((cs.hash[6] << 8 ) & 0x000000000000FF00U)
        |  (cs.hash[7] & 0x00000000000000FFU);
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

EOSIO_ABI( eosdacrandom, (setsize) (sendseed) (sendhash) (regrequest) )
