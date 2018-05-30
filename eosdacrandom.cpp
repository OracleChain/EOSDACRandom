#include "edtrandom.hpp"
#include <eosiolib/system.h>
#include "../EOSDACVote/eosdacvote.hpp"
#include "../EOSDACToken/eosdactoken.hpp"

using namespace eosio;

eosdacrandom::eosdacrandom(account_name name)
        : contract(name),
          _self(name),
          _seeds(_self, name),
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
    eosio_assert(_seeds_count == _seed_target_size, "some seed hash has not been set");
    eosio::asset fromBalance = eosdactoken(tokenContract).get_balance(owner, string_to_name(symbol.c_str()));
    eosio_assert(fromBalance.amount > 0, "account has not enough OCT to do it");

    eosio::asset selfBalance = eosdactoken(tokenContract).get_balance(_self, string_to_name(symbol.c_str()));
    eosio_assert(selfBalance.amount > 0, "contract account has not enough OCT to do it");

    require_auth(owner);

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

    auto s = _seeds.find(owner);
    eosio_assert(s != _seeds.end(), "account not found");
    eosio_assert(s->seed != seed, "you have already send seed");

    if (s.hah != h) {
        print("seed not match hash");
        SEND_INLINE_ACTION( eosdacvote, vote, {_self,N(active)}, {_self, owner, selfBalance, false} );
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
		int64_t num = random();
		uint64_t cur = current_time();
		static int expiraion = 3000; // ms

        for (auto i = _geters.cbegin(); i != _geters.cend();) {
            if (cur - i->timestamp >= expiraion) {
                SEND_INLINE_ACTION( i->owner, getrandom, {_self,N(active)}, {num} );
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
    eosio::asset fromBalance = eosdactoken(tokenContract).get_balance(owner, string_to_name(symbol.c_str()));
    eosio_assert(fromBalance.amount > 0, "account has not enough OCT to do it");

    required_auth(owner);

    auto s = _seeds.find(owner);
    if (s == _seeds.end()) {
        _seeds.emplace(_self, [&](auto& a){
            a.owner = owner;
            a.hash = hash;
            _seeds_count++;
        });
    } else {
        _seeds.modify(s, _self, [&](auto& a){
            a.hash = hash;
        });
    }
}

void eosdacrandom::getrandom(name owner)
{
    eosio_assert(is_account(owner));

    uint64_t cur = current_time();

    auto it = _geters.find(owner);
    if (it == _geters.end()) {
        _geters.emplace(_self, [&](auto& a){
            a.owner = owner;
            a.timestamp = cur;
        });
    } else {
        _geters.modify(it, _self, [&](auto& a){
            a.timestamp = cur;
        });
    }
}

int64_t eosdacrandom::random()
{
    // use _seeds to generate random number
    eosio_assert(_seeds_count == _seed_target_size, "seed is not full");

    int64_t  seed = 0;
    for (auto it = _seeds.cbegin(); it != _seeds.cend();) {
        seed += it->seed;
        it = _seeds.erase(it);
    }

    _seeds_count = 0;
    _seeds_match = 0;

    srand48(seed);
    return rand();
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

EOSIO_ABI( eosdacrandom, (setsize) (sendseed) (sendhash) (getrandom) )
