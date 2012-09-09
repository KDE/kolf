#! /bin/sh
#This file outputs in a separate line each file with a .desktop syntax
#that needs to be translated but has a non .desktop extension
for i in `cat courses.list`; do echo $i; done
