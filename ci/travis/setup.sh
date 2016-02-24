#!/bin/bash

eigenVersion=3.2.8.tar.bz2
wget http://bitbucket.org/eigen/eigen/get/${eigenVersion}
tar xvf ${eigenVersion}
mv eigen-* eigen
