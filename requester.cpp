#include <eosiolib/eosio.hpp>
using namespace eosio;

class requester : public eosio::contract {
public:
	using contract::contract;

	// @abi action
	void genrandom(string orderid, uint64_t num) {
		print("Order id: ");
		print(orderid);
		print("Random number: ");
		print(num);
	}
};

EOSIO_ABI( requester, (genrandom) )
