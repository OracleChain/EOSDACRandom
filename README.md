# EOSDACRandom

A random number generator smart contract.

### general steps of usage

1. setup eosdacrandom contract
2. run randomseed server
3. define random number handler action in your contract
3. send random number request and get response with random number handler asynchronously

### setup instruction

1. compile the contract through

```bash
make
```

2. set up the contract through

```bash
make install
```

3. test the random generator through

```bash
make test
```

### how to define customized random number handler

after the random number request is sent, the random generator will call the given random number handler when random number is generated. you need to predefine an action with sign of 

```c++
void func(uint64_t index, uint64_t num);
```

the name of the handler can be chosen at will.
