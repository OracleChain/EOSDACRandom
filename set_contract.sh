#!/bin/bash

#key for random account
cleos --wallet-url http://localhost:8899 -u http://localhost:8788 wallet import 5KHa8jpUFG771Qvxzq2ND7pBmMsBSWLQiWQexh8w5BBeEbUUx3M
#key for seed accounts
cleos --wallet-url http://localhost:8899 -u http://localhost:8788 wallet import 5JT4L5vz3HBLd1NisFD1RrM85oeFq7axc58271cKD1J9sBid1bk
cleos --wallet-url http://localhost:8899 -u http://localhost:8788 wallet import 5JLt7YrPbREMBgo6X5HXK3Vq3Sn1CFJM4JYPdtzWh2bruUBDN6w
cleos --wallet-url http://localhost:8899 -u http://localhost:8788 wallet import 5JxFc9aMB9f7B2JNQ9oUGv1q7w9TDL8HUMwCFVt5BD8Rga61HjN
#key for random requester
cleos --wallet-url http://localhost:8899 -u http://localhost:8788 wallet import 5JSomFPK8F5sZKN5P4FEVwfACRsv3zKPDveL5w53HCLLiJXViiF
#create accounts
cleos --wallet-url http://localhost:8899 -u http://localhost:8788 create account eosio eosdacrandom EOS55MfVEkczBuFkbh9XyGTurk3KSAu9RcA5VTTEbwbcFdsHmhzGK -p eosio
cleos --wallet-url http://localhost:8899 -u http://localhost:8788 create account eosdacrandom seed1 EOS8NfQjUMLoj89yUWz3VpZECGM3HvFXinJbTaifGTt4PX3m3f8Ye -p eosdacrandom
cleos --wallet-url http://localhost:8899 -u http://localhost:8788 create account eosdacrandom seed2 EOS6y1ZGm1wst7PVmhhr9fiUeJuVfiba6URdgiXQbkU3k4DP3fHwL -p eosdacrandom
cleos --wallet-url http://localhost:8899 -u http://localhost:8788 create account eosdacrandom seed3 EOS7bSXhVoHdipW1GHjPmbB1LZDVvxCbrMCie7DtXdvx8hTjLoUFb -p eosdacrandom
cleos --wallet-url http://localhost:8899 -u http://localhost:8788 create account eosdacrandom requester EOS789Yo4bDT1afi3B9xk47Y8sQd8tqH4jGqc5Bi8BxTJumjr87eM -p eosdacrandom
#set contract
cleos --wallet-url http://localhost:8899 -u http://localhost:8788 set contract eosdacrandom ./eosdacrandom -p eosdacrandom
#set authority
cleos --wallet-url http://localhost:8899 -u http://localhost:8788 set account permission eosdacrandom active '{"threshold":1,"keys":[{"key":"EOS55MfVEkczBuFkbh9XyGTurk3KSAu9RcA5VTTEbwbcFdsHmhzGK","weight":1}],"accounts":[{"permission":{"actor":"eosdacrandom","permission":"eosio.code"},"weight":1}],"waits":[]}' owner -p eosdacrandom -x 10
#set test contract
cleos --wallet-url http://localhost:8899 -u http://localhost:8788 set contract requester ./requester -p requester

