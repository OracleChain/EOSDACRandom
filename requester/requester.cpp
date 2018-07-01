#include <eosiolib/eosio.hpp>
using namespace eosio;

class requester : public eosio::contract {
public:
	using contract::contract;

	// @abi action
	void getrandom(uint64_t index, uint64_t num) {
		print("Index: ");
		print(index);
		print("Random number: ");
		print(num);
	}
};

EOSIO_ABI( requester, (getrandom) )
