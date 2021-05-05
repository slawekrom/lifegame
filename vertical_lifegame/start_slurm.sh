#!/bin/bash
sudo /etc/init.d/munge start
sudo service slurmd start
sudo service slurmctld start
