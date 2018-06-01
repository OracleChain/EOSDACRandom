#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>
#include <string>

using namespace eosio;
using namespace std;

// @abi table seed_info
struct seed_info
{
    name    owner;
    int64_t seed;
    string  hash;

    uint64_t primary_key() const { return owner; }

    EOSLIB_SERIALIZE( seed_info, (owner) (seed) (hash))
};

typedef eosio::multi_index<N(seeds), seed_info> seed_table;

// @abi table geter_info
struct geter_info
{
    name owner;
    uint64_t index;
    uint64_t timestamp;

    uint64_t primary_key() const { return owner; }

    EOSLIB_SERIALIZE( geter_info, (owner) (timestamp) )
};

typedef  eosio::multi_index<N(geters), geter_info> geter_table;

// void getrandom(uint64_t index, int64_t number) {}

class eosdacrandom : public eosio::contract
{
public:
    eosdacrandom(account_name name);
    ~eosdacrandom();

    // @abi action
    void setsize(uint64_t size);

    // @abi action
    void sendseed(name owner, int64_t seed, string symbol);

    // @abi action
    void sendhash(name owner, string hash, string symbol);

    // @abi action
    void getrandom(name owner, uint64_t index);

private:
    int64_t random();
    bool seedsmatch();

private:
    seed_table _seeds;
    geter_table _geters;
    uint64_t _seed_target_size;
    uint64_t _seeds_count;
    uint64_t _seeds_match;
};
