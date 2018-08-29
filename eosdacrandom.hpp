#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>
#include <string>
#include <vector>
#include <tuple>

using namespace eosio;
using namespace std;

// @abi table
struct seedinfo
{
    name    datafeeder;
    string  seed;
    string  hash;

    uint64_t primary_key() const { return datafeeder.value; }

    EOSLIB_SERIALIZE( seedinfo, (datafeeder) (seed) (hash))
};

typedef eosio::multi_index<N(seedinfo), seedinfo> seed_table;

// @abi table
struct requestinfo
{
    uint64_t    timestamp;
    string      orderid;

    EOSLIB_SERIALIZE( requestinfo, (timestamp) (orderid) )
};

// @abi table
struct geterinfo
{
    name                    consumer;
    vector<requestinfo>     requestinfos;

    uint64_t primary_key() const { return consumer.value; }

    EOSLIB_SERIALIZE( geterinfo, (consumer) (requestinfos) )
};

typedef  eosio::multi_index<N(geterinfo), geterinfo> geter_table;

// void getrandom(string orderid, int64_t number) {}

class eosdacrandom : public eosio::contract
{
public:
    eosdacrandom(account_name name);
    ~eosdacrandom();

    // @abi action
    void setsize(uint64_t size);

    // @abi action
    void setreserved(vector<name> dfs);

    // @abi action
    void sendseed(name datafeeder, string seed);

    // @abi action
    void sendhash(name datafeeder, string hash);

    // @abi action
    void regrequest(name consumer, string orderid);

public:

    // @abi table
    struct seedconfig
    {
        account_name owner;
        uint64_t target_size;
        uint64_t hash_count;
        uint64_t seed_match;

        uint64_t primary_key() const { return owner; }

        EOSLIB_SERIALIZE( seedconfig, (owner) (target_size) (hash_count) (seed_match) )
    };

    typedef eosio::multi_index<N(seedconfig), seedconfig> seedconfig_table;

private:
    string one_seed();
    bool seedsmatch();

    checksum256 cal_sha256(int index, string orderid, string seed);
    string cal_sha256_str(string word);
    string make_sha256_str(int index, string orderid, string seed);

    void dispatch_request();
};
