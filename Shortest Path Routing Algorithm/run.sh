#!/bin/bash

#./nse-linux386 localhost 1294 &

./router 1 129.97.167.26 1294 3323 &
./router 2 129.97.167.26 1294 3324 &
./router 3 129.97.167.26 1294 3325 &
./router 4 129.97.167.26 1294 3326 &
./router 5 129.97.167.26 1294 3327 &

# ps aux | grep router | awk '{print $2}' | xargs kill
