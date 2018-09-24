#!/bin/bash

echo "Dumping env..."
env

if [ -n "$REPLY_FD" ]; then
  REPLY_FD="/proc/self/fd/$REPLY_FD"
  echo "Got a reply fd... sending data back up to the daemon..."
  echo "Hello world from the script!" >> $REPLY_FD
fi

exit 0
