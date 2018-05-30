# EOSDACRandom
A random number generator smart contract.

### participator
1. random contract (aka contract)
2. provider
3. customer

### procedure
#### contract
  * set seed size, e.g n.

#### providers
  * all n providers send hash of seed, e.g h1.
  * all providers send seed, e.g s, (MUST wait for h1 sent, otherwise this will fail).
  * calculate hash of seed s, get h2, compare h1 and h2, if equals, we put seed s in seed pool, otherwise, put provider in blacklist.

#### customers
  * first need implment a `getrandom` interface.
  * want a random number, register himself to random contract.
  * random contract generator a random number using seed pool. After it's done, clear pool, wait for next round.
  * send customer this random number using `getrandom` by `SEND_INLINE_ACTION`. After it's done, clear geters table, wait for next round.

