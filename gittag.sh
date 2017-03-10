#!/bin/bash

if [ $# -lt 1 ]; then
   echo "Usage: gittag <tagname>" && exit 1
fi

#echo $1 > VERSION
#git add VERSION
git commit -m "Set version to $1"
git push origin
git tag -a $1 -m "Tagged version:$1"
git push --tags

