#!/bin/bash
index=0
while : ; do
	cleos --wallet-url http://localhost:8899 -u http://localhost:8788 push action eosdacrandom regrequest "{\"owner\":\"requester\",\"index\":$index,\"handler\":\"getrandom\"}" -p requester
	index=`expr "$index" + 1`
	sleep 1s
done
