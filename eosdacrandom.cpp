#include "eosdacrandom.hpp"
#include <eosiolib/system.h>
#include <eosiolib/stdlib.hpp>
#include <eosiolib/action.hpp>
#include "../EOSDACVote/eosdacvote.hpp"
#include "../EOSDACToken/eosdactoken.hpp"

using namespace eosio;

eosdacrandom::eosdacrandom(account_name name)
        : contract(name),
          _seeds(_self, name),
          _geters(_self, name),
          _seed_target_size(3),
          _seeds_count(0),
          _seeds_match(0)
{

}

eosdacrandom::~eosdacrandom()
{

}

void eosdacrandom::setsize(uint64_t size)
{
    require_auth(_self);
    eosio_assert(_seeds_count < _seed_target_size, "seeds full, do not change size now");
    _seed_target_size = size;
}

void eosdacrandom::sendseed(name owner, int64_t seed, string symbol)
{
    eosio_assert(is_account(owner), "Invalid account");
    eosio::asset fromBalance = eosdactoken(N(octoneos)).get_balance(owner, string_to_name(symbol.c_str()));
    eosio_assert(fromBalance.amount > 0, "account has not enough OCT to do it");

    eosio::asset selfBalance = eosdactoken(N(octoneos)).get_balance(_self, string_to_name(symbol.c_str()));
    eosio_assert(selfBalance.amount > 0, "contract account has not enough OCT to do it");

    require_auth(owner);

    auto s = _seeds.find(owner);
    if (_seeds_count < _seed_target_size) {
        if (s != _seeds.end()) {
            _seeds.erase(s);
            _seeds_count --;
        }
        eosio_assert(s != _seeds.end(), "account not found");
    }

    checksum256 cs;
    char d[255] = { 0 };
    sprintf(d, "%I64d", seed);
    sha256(d, strlen(d), &cs);

    string h;
    for (int i = 0; i < sizeof(cs.hash); ++i) {
        char hex[3];
        sprintf((char*)&hex[0], "%02x", cs.hash[i]);
        h += hex;
    }

    eosio_assert(s->seed != seed, "you have already send seed");
    if (s->hash != h) {
        print("seed not match hash");
        //SEND_INLINE_ACTION( eosdacvote, vote, {_self,N(active)}, {_self, owner, selfBalance, false} );
        for (auto itr = _seeds.cbegin(); itr != _seeds.cend(); ) {
            itr = _seeds.erase(itr);
        }
        _seeds_count = 0;
        return;
    }

    _seeds.modify(s, _self, [&](auto& a){
        a.seed = seed;
    });

    _seeds_match ++;

    // if all seeds match
    if (seedsmatch()) {
        uint64_t cur = current_time();
        int64_t num = random();
		static int expiraion = 3000; // ms

        for (auto i = _geters.begin(); i != _geters.end();) {
            auto& v = i->requestinfo;
            for (auto j = v.cbegin(); j != v.cend();) {
                if (cur - std::get<1>(*j) >= expiraion) {
                    dispatch_inline(i->owner, string_to_name("getrandom"), {permission_level(_self, N(active))}, std::make_tuple(std::get<0>(*j), num));
                    j = v.erase(j);
                } else {
                    ++j;
                }
            }

            if (i->requestinfo.size() == 0) {   // if all request clear, erase the account.
                i = _geters.erase(i);
            } else {
                ++i;
            }
        }
    }
}

void eosdacrandom::sendhash(name owner, string hash, string symbol)
{
    eosio_assert(is_account(owner), "Invalid account");
    eosio_assert(_seeds_count < _seed_target_size, "seeds is full");
    eosio::asset fromBalance = eosdactoken(N(octoneos)).get_balance(owner, string_to_name(symbol.c_str()));
    eosio_assert(fromBalance.amount > 0, "account has not enough OCT to do it");

    require_auth(owner);

    auto s = _seeds.find(owner);
    if (s == _seeds.end()) {
        _seeds.emplace(_self, [&](auto& a){
            a.owner = owner;
            a.hash = hash;
        });
        _seeds_count++;
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
    auto req = std::make_tuple(index, cur);
    if (it == _geters.end()) {
        _geters.emplace(_self, [&](auto& a){
            a.owner = owner;
            a.requestinfo.push_back(req);
        });
    } else {
        _geters.modify(it, _self, [&](auto& a){
            bool same_idx = false;
            for (auto ri = a.requestinfo.begin(); ri != a.requestinfo.end(); ++ri) {
                if (std::get<0>(*ri) == index) {    // if index equals former index.
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
}

int64_t eosdacrandom::random()
{
    // use _seeds to generate random number
    eosio_assert(_seeds_count == _seed_target_size, "seed is not full");

    // how to generate random number?
    int64_t  seed = 0;
    for (auto it = _seeds.cbegin(); it != _seeds.cend();) {
        seed += it->seed;
        it = _seeds.erase(it);
    }

    _seeds_count = 0;
    _seeds_match = 0;

    checksum256 cs;
    char d[255] = { 0 };
    sprintf(d, "%I64d", seed);
    sha256(d, strlen(d), &cs);

    return ((cs.hash[0] & 0xFF) |
            ((cs.hash[1] & 0xFF) << 8) |
            ((cs.hash[2] & 0xFF) << 16) |
            ((cs.hash[3] & 0xFF) << 24) |
            ((cs.hash[4] & 0xFF) << 32) |
            ((cs.hash[5] & 0xFF) << 40) |
            ((cs.hash[6] & 0xFF) << 48) |
            ((cs.hash[7] & 0xFF) << 56));
}

bool eosdacrandom::seedsmatch()
{
    if (_seeds_count != _seed_target_size) {
        return false;
    }

    if (_seeds_count == _seeds_match) {
        return true;
    }

    return false;
}

EOSIO_ABI( eosdacrandom, (setsize) (sendseed) (sendhash) (regrequest) )
