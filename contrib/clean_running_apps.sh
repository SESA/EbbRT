#!/bin/bash

# Manual clean-up of docker networks and containers created by the EbbRT NodeAllocator

read -p "Remove all of your ($USER) ebbrt networks and nodes. Continue [Y]es, [n]o? " c
case "$c" in 
  n|N ) exit 0;
esac

echo "Removing docker containers..."
docker ps | grep ebbrt-$(id -u) | cut -d ' ' -f 1 | while read id; do docker kill $id; done

echo "Removing docker networks..."
docker network ls | grep ebbrt-$(id -u) | cut -d ' ' -f 9 | while read id; do docker network rm $id; done

