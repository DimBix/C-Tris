#!/bin/bash

pid=$(pgrep TriServer);
if [ $? -eq 1 ]
then 
  echo -1
else 
  ipcs -m -p | grep --no-messages $pid 2> /dev/null | cut -d " " -f 1;
  echo $pid
fi
