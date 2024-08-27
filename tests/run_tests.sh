#!/bin/sh
set -ue
cd "$(dirname "$0")"
export G_DEBUG=
env | sort > /tmp/foo
for i in test*.py; do
  ./"$i"
done
