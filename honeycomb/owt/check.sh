#!/bin/bash
ls beacons | cut -f2 -d"_" | cut -f1 -d'.' | sort | uniq -c | sort -rn
