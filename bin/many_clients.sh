#!/bin/bash

set -euo pipefail
#set -x


for i in {1..1000}
do
   ./build/client localhost 9999 "hello!"
done
