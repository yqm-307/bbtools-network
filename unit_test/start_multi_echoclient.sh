#!/bin/bash

for ((i=1; i<100; ++i))
do
    ./echo_client 127.0.0.1 10010
done